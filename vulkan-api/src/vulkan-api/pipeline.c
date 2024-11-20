/* Copyright (c) 2024 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "pipeline.h"

#include "context.h"
#include "pipeline_cache.h"
#include "program_manager.h"
#include "renderpass.h"
#include "shader.h"

vkapi_graphics_pl_t vkapi_graph_pl_create(
    vkapi_context_t* context, graphics_pl_key_t* key, struct SpecConstParams* spec_consts)
{
    vkapi_graphics_pl_t pl;

    // sort the vertex attribute descriptors so only ones that are used
    // are applied to the pipeline
    VkVertexInputAttributeDescription input_decs[VKAPI_PIPELINE_MAX_VERTEX_ATTR_COUNT] = {0};
    int input_desc_count = 0;
    for (int i = 0; i < VKAPI_PIPELINE_MAX_VERTEX_ATTR_COUNT; ++i)
    {
        if (key->vert_attr_descs[i].format != VK_FORMAT_UNDEFINED)
        {
            input_decs[input_desc_count++] = key->vert_attr_descs[i];
        }
    }

    bool hasInputState = !input_desc_count;
    VkPipelineVertexInputStateCreateInfo vis = {0};
    vis.vertexAttributeDescriptionCount = input_desc_count;
    vis.pVertexAttributeDescriptions = hasInputState ? input_decs : VK_NULL_HANDLE;
    vis.vertexBindingDescriptionCount = hasInputState ? 1 : 0;
    vis.pVertexBindingDescriptions = hasInputState ? key->vert_bind_descs : VK_NULL_HANDLE;

    key->asm_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

    key->ds_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    key->ds_state.depthBoundsTestEnable = VK_FALSE;
    key->ds_state.minDepthBounds = 0.0f;
    key->ds_state.maxDepthBounds = 0.0f;

    key->raster_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    key->raster_state.lineWidth = 1.0f;
    key->raster_state.depthClampEnable = VK_FALSE;
    key->raster_state.depthBiasConstantFactor = 0.0f;
    key->raster_state.rasterizerDiscardEnable = VK_FALSE;
    key->raster_state.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo ms = {0};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

    // ============ dynamic states ====================
    VkPipelineDynamicStateCreateInfo dcs = {0};
    VkDynamicState states[2] = {VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT};
    dcs.dynamicStateCount = 2;
    dcs.pDynamicStates = states;

    // =============== viewport state ====================
    // scissor and viewport are set at draw time
    VkPipelineViewportStateCreateInfo vs = {0};
    vs.viewportCount = 1;
    vs.scissorCount = 1;

    // =============== tesselation =======================
    VkPipelineTessellationStateCreateInfo tsc = {0};
    tsc.patchControlPoints = key->tesse_vert_count;

    // ============= colour attachment =================
    // all blend attachments are the same for each pass
    VkPipelineColorBlendStateCreateInfo cbs = {0};
    VkPipelineColorBlendAttachmentState bas[VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT] = {0};
    for (uint32_t i = 0; i < key->colour_attach_count; ++i)
    {
        bas[i] = key->blend_state;
    }
    cbs.attachmentCount = key->colour_attach_count;
    cbs.pAttachments = bas;

    // We only want to add valid shaders to the pipeline. Because the key states
    // all shaders whether they are required or not, we need to create a
    // container with only valid shaders which is checked by testing whether the
    // name field of the create-info is not NULL.
    VkPipelineShaderStageCreateInfo shaders[RPE_BACKEND_SHADER_STAGE_MAX_COUNT];
    VkSpecializationInfo spi[RPE_BACKEND_SHADER_STAGE_MAX_COUNT] = {0};

    uint32_t shader_count = 0;
    for (int i = 0; i < RPE_BACKEND_SHADER_STAGE_MAX_COUNT; ++i)
    {
        if (key->shaders[i].pName)
        {
            shaders[shader_count] = shaders[i];

            if (key->spec_map_entry_count[i] > 0)
            {
                spi[i].dataSize = spec_consts[i].data_size;
                spi[i].mapEntryCount = key->spec_map_entry_count[i];
                spi[i].pMapEntries = key->spec_map_entries[i];
                assert(spec_consts[i].data);
                spi[i].pData = spec_consts[i].data;

                // Add the specialisation constant info to the shader module.
                key->shaders[shader_count++].pSpecializationInfo = &spi[i];
            }
        }
    }
    assert(shader_count);

    VkGraphicsPipelineCreateInfo ci = {0};
    ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    ci.stageCount = shader_count;
    ci.pStages = shaders;
    ci.pVertexInputState = &vis;
    ci.pInputAssemblyState = &key->asm_state;
    ci.pTessellationState = key->tesse_vert_count > 0 ? &tsc : VK_NULL_HANDLE;
    ci.pViewportState = &vs;
    ci.pRasterizationState = &key->raster_state;
    ci.pMultisampleState = &ms;
    ci.pDepthStencilState = &key->ds_state;
    ci.pColorBlendState = &cbs;
    ci.pDynamicState = &dcs;
    ci.layout = key->pl_layout;
    ci.renderPass = key->render_pass;

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(
        context->device, VK_NULL_HANDLE, 1, &ci, VK_NULL_HANDLE, &pl.instance));

    return pl;
}

vkapi_compute_pl_t vkapi_compute_pl_create(vkapi_context_t* context, compute_pl_key_t* key)
{
    assert(key);
    assert(key->pl_layout);

    vkapi_compute_pl_t pl = {0};
    VkComputePipelineCreateInfo ci = {0};
    ci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    ci.layout = key->pl_layout;
    ci.stage = key->shader;
    VK_CHECK_RESULT(vkCreateComputePipelines(
        context->device, VK_NULL_HANDLE, 1, &ci, VK_NULL_HANDLE, &pl.instance));
    return pl;
}
