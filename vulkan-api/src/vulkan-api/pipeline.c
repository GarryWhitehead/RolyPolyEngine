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
#include "driver.h"
#include "program_manager.h"
#include "renderpass.h"
#include "shader.h"

#include <string.h>
#include <utility/hash.h>

/**
 All the data required to create a pipeline layout
 */
typedef struct PipelineLayout
{
    // each descriptor type has its own set.
    hash_set_t desc_bindings;
    uint32_t desc_binding_counts[VKAPI_PIPELINE_MAX_DESC_SET_LAYOUT_BINDING_COUNT];

    VkDescriptorSetLayout desc_layouts[VKAPI_PIPELINE_MAX_DESC_SET_COUNT];

    // the shader stage the push constant refers to and its size
    size_t push_constant_sizes[RPE_BACKEND_SHADER_STAGE_MAX_COUNT];
    VkPipelineLayout instance;

} vkapi_pl_layout_t;

typedef struct GraphicsPipeline
{
    // dynamic states to be used with this pipeline - by default the viewport
    // and scissor dynamic states are set
    VkDynamicState dyn_states[6];
    uint32_t dyn_state_count;

    VkPipeline instance;
} vkapi_graphics_pl_t;

typedef struct ComputePipeline
{
    VkPipeline instance;

} vkapi_compute_pl_t;

/** Pipeline layout functions **/

vkapi_pl_layout_t* vkapi_pl_layout_init(arena_t* arena)
{
    vkapi_pl_layout_t* out = ARENA_MAKE_STRUCT(arena, vkapi_pl_layout_t, ARENA_ZERO_MEMORY);
    out->desc_bindings =
        HASH_SET_CREATE(uint32_t, VkDescriptorSetLayoutBinding*, arena, murmur_hash3);
    assert(out);
    return out;
}

void vkapi_pl_build(vkapi_pl_layout_t* layout, vkapi_context_t* context)
{
    if (layout->instance)
    {
        return;
    }

    // Generate the descriptor set layouts for each binding.
    for (uint32_t set_idx = 0; set_idx < VKAPI_PIPELINE_MAX_DESC_SET_COUNT; ++set_idx)
    {
        VkDescriptorSetLayoutBinding* set_binding = HASH_SET_GET(&layout->desc_bindings, &set_idx);
        VkDescriptorSetLayoutCreateInfo layout_info = {0};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = layout->desc_binding_counts[set_idx];
        layout_info.pBindings = !layout_info.bindingCount ? VK_NULL_HANDLE : set_binding;
        vkCreateDescriptorSetLayout(
            context->device, &layout_info, VK_NULL_HANDLE, &layout->desc_layouts[set_idx]);
    }

    // create push constants - just the size for now. The data contents are set
    // at draw time
    VkPushConstantRange constant_ranges[VKAPI_PIPELINE_MAX_PUSH_CONSTANT_COUNT];
    uint32_t cr_count = 0;

    for (int i = 0; i < RPE_BACKEND_SHADER_STAGE_MAX_COUNT; ++i)
    {
        if (layout->push_constant_sizes[i])
        {
            VkPushConstantRange pcr = {
                .size = layout->push_constant_sizes[i],
                .offset = 0,
                .stageFlags = shader_vk_stage_flag(i)};
            constant_ranges[cr_count++] = pcr;
        }
    }

    VkPipelineLayoutCreateInfo pl_info = {0};
    pl_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pl_info.pushConstantRangeCount = cr_count;
    pl_info.pPushConstantRanges = constant_ranges;
    pl_info.setLayoutCount = VKAPI_PIPELINE_MAX_DESC_SET_COUNT;
    pl_info.pSetLayouts = layout->desc_layouts;

    VK_CHECK_RESULT(
        vkCreatePipelineLayout(context->device, &pl_info, VK_NULL_HANDLE, &layout->instance));
}

void vkapi_pl_layout_add_push_const(vkapi_pl_layout_t* layout, enum ShaderStage stage, size_t size)
{
    layout->push_constant_sizes[stage] = size;
}

void vkapi_pl_layout_bind_push_block(
    vkapi_pl_layout_t* layout, VkCommandBuffer cmdBuffer, push_block_bind_params_t* push_block)
{
    vkCmdPushConstants(
        cmdBuffer, layout->instance, push_block->stage, 0, push_block->size, push_block->data);
}

void vkapi_pl_layout_add_desc_layout(
    vkapi_pl_layout_t* layout,
    uint32_t set,
    uint32_t binding,
    VkDescriptorType desc_type,
    VkShaderStageFlags stage,
    arena_t* arena)
{
    assert(set < VKAPI_PIPELINE_MAX_DESC_SET_COUNT);
    VkDescriptorSetLayoutBinding set_binding = {0};
    set_binding.binding = binding;
    set_binding.descriptorType = desc_type;
    set_binding.descriptorCount = 1;
    set_binding.stageFlags = stage;

    VkDescriptorSetLayoutBinding* slb = HASH_SET_GET(&layout->desc_bindings, &set);
    if (slb)
    {
        int bind_idx = INT32_MAX;
        for (int i = 0; i < VKAPI_PIPELINE_MAX_DESC_SET_LAYOUT_BINDING_COUNT; ++i)
        {
            if (slb[i].binding == binding)
            {
                bind_idx = i;
                break;
            }
        }
        if (bind_idx != INT32_MAX)
        {
            assert(slb[bind_idx].descriptorType == desc_type);
            slb[bind_idx].stageFlags |= stage;
            return;
        }
    }
    else
    {
        slb = ARENA_MAKE_ARRAY(
            arena,
            VkDescriptorSetLayoutBinding,
            VKAPI_PIPELINE_MAX_DESC_SET_LAYOUT_BINDING_COUNT,
            0);
        HASH_SET_INSERT(&layout->desc_bindings, &set, &slb);
    }
    assert(layout->desc_binding_counts[set] < VKAPI_PIPELINE_MAX_DESC_SET_LAYOUT_BINDING_COUNT);
    slb[layout->desc_binding_counts[set]++] = set_binding;
}

void vkapi_pl_clear_descs(vkapi_pl_layout_t* layout)
{
    assert(layout);
    memset(
        layout->desc_binding_counts,
        0,
        sizeof(uint32_t) * VKAPI_PIPELINE_MAX_DESC_SET_LAYOUT_BINDING_COUNT);
    hash_set_clear(&layout->desc_bindings);
}

// ================== pipeline =======================

void vkapi_graph_pl_create(
    vkapi_context_t* context,
    vkapi_graphics_pl_t* pipeline,
    vkapi_pl_layout_t* layout,
    graphics_pl_key_t* key)
{
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

    // ============== primitive topology =====================
    VkPipelineInputAssemblyStateCreateInfo as = {0};
    as.topology = key->raster_state.topology;
    as.primitiveRestartEnable = key->raster_state.prim_restart;

    // ============== multi-sample state =====================
    VkPipelineMultisampleStateCreateInfo ss = {0};

    // ============== depth/stenicl state ====================
    VkPipelineDepthStencilStateCreateInfo dss = {0};
    dss.depthTestEnable = key->raster_state.depth_test_enable;
    dss.depthWriteEnable = key->raster_state.depth_write_enable;
    dss.depthCompareOp = key->ds_block.compare_op;

    // ============== stencil state =====================
    dss.stencilTestEnable = key->ds_block.stencil_test_enable;
    if (key->ds_block.stencil_test_enable)
    {
        dss.front.failOp = key->ds_block.stencil_fail_op;
        dss.front.depthFailOp = key->ds_block.depth_fail_op;
        dss.front.passOp = key->ds_block.pass_op;
        dss.front.compareMask = key->ds_block.compare_mask;
        dss.front.writeMask = key->ds_block.write_mask;
        dss.front.reference = key->ds_block.reference;
        dss.front.compareOp = key->ds_block.compare_op;
        dss.back = dss.front;
    }

    // ============ raster state =======================
    VkPipelineRasterizationStateCreateInfo rs = {0};
    rs.cullMode = key->raster_state.cull_mode;
    rs.frontFace = key->raster_state.front_face;
    rs.polygonMode = key->raster_state.polygon_mode;
    rs.lineWidth = 1.0f;

    // ============ dynamic states ====================
    VkPipelineDynamicStateCreateInfo dcs = {0};
    dcs.dynamicStateCount = pipeline->dyn_state_count;
    dcs.pDynamicStates = pipeline->dyn_states;

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
    for (uint32_t i = 0; i < key->raster_state.colour_attach_count; ++i)
    {
        bas[i].srcColorBlendFactor = key->blend_state.src_color_blend_factor;
        bas[i].dstColorBlendFactor = key->blend_state.dst_color_blend_factor;
        bas[i].colorBlendOp = key->blend_state.color_blend_op;
        bas[i].srcAlphaBlendFactor = key->blend_state.src_alpha_blend_factor;
        bas[i].dstAlphaBlendFactor = key->blend_state.dst_alpha_blend_factor;
        bas[i].alphaBlendOp = key->blend_state.alpha_blend_op;
        bas[i].colorWriteMask = key->raster_state.color_write_mask;
        bas[i].blendEnable = key->blend_state.blend_enable;
        assert(key->blend_state.blend_enable <= 1);
    }
    cbs.attachmentCount = key->raster_state.colour_attach_count;
    cbs.pAttachments = bas;

    // ================= create the pipeline =======================
    assert(layout->instance);

    // We only want to add valid shaders to the pipeline. Because the key states
    // all shaders whether they are required or not, we need to create a
    // container with only valid shaders which is checked by testing whether the
    // name field of the create-info is not NULL.
    VkPipelineShaderStageCreateInfo shaders[RPE_BACKEND_SHADER_STAGE_MAX_COUNT];
    uint32_t shader_count = 0;
    for (int i = 0; i < RPE_BACKEND_SHADER_STAGE_MAX_COUNT; ++i)
    {
        if (key->shaders[i].pName)
        {
            shaders[shader_count++] = shaders[i];
        }
    }
    assert(shader_count);

    VkGraphicsPipelineCreateInfo ci = {0};
    ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    ci.stageCount = shader_count;
    ci.pStages = shaders;
    ci.pVertexInputState = &vis;
    ci.pInputAssemblyState = &as;
    ci.pTessellationState = key->tesse_vert_count > 0 ? &tsc : VK_NULL_HANDLE;
    ci.pViewportState = &vs;
    ci.pRasterizationState = &rs;
    ci.pMultisampleState = &ss;
    ci.pDepthStencilState = &dss;
    ci.pColorBlendState = &cbs;
    ci.pDynamicState = &dcs;
    ci.layout = layout->instance;
    ci.renderPass = key->render_pass;

    VK_CHECK_RESULT(vkCreateGraphicsPipelines(
        context->device, VK_NULL_HANDLE, 1, &ci, VK_NULL_HANDLE, &pipeline->instance));
}

void vkapi_compute_pl_create(
    vkapi_compute_pl_t* pipeline,
    vkapi_context_t* context,
    compute_pl_key_t* key,
    vkapi_pl_layout_t* layout)
{
    assert(layout);
    assert(key);
    assert(pipeline);

    VkComputePipelineCreateInfo ci = {0};
    ci.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    ci.layout = layout->instance;
    ci.stage = key->shader;
    VK_CHECK_RESULT(vkCreateComputePipelines(
        context->device, VK_NULL_HANDLE, 1, &ci, VK_NULL_HANDLE, &pipeline->instance));
}
