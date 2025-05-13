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

#include "backend/enums.h"
#include "common.h"
#include "descriptor_cache.h"
#include "pipeline.h"

#include <utility/hash_set.h>

#ifndef __VKAPI_PIPELINE_CACHE_H__
#define __VKAPI_PIPELINE_CACHE_H__

typedef struct GraphicsPipeline vkapi_graphics_pl_t;
typedef struct PipelineLayout vkapi_pl_layout_t;
typedef struct ShaderProgramBundle shader_prog_bundle_t;
struct SpecConstParams;

typedef struct PipelineLayoutKey
{
    struct PushBlockInfo
    {
        size_t size;
        size_t stage;
    } push_block_info[RPE_BACKEND_SHADER_STAGE_MAX_COUNT];
    VkDescriptorSetLayout layouts[VKAPI_PIPELINE_MAX_DESC_SET_COUNT];
} pl_layout_key_t;

typedef struct PipelineLayout
{
    VkPipelineLayout instance;
    uint64_t frame_last_used;
} vkapi_pl_layout_t;

RPE_PACKED(typedef struct GraphicsPipelineKey {
    struct RasterStateBlock
    {
        VkCullModeFlagBits cull_mode;
        VkPolygonMode polygon_mode;
        VkFrontFace front_face;
        VkPrimitiveTopology topology;
        VkColorComponentFlags colour_write_mask;
        VkBool32 prim_restart;
        VkBool32 depth_test_enable;
        VkBool32 depth_write_enable;
        VkBool32 depth_clamp_enable;
        VkCompareOp depth_compare_op;
    } raster_state;

    struct DepthStencilBlock
    {
        VkCompareOp compare_op;
        VkStencilOp stencil_fail_op;
        VkStencilOp depth_fail_op;
        VkStencilOp pass_op;
        uint32_t compare_mask;
        uint32_t write_mask;
        uint32_t reference;
        VkBool32 stencil_test_enable;
    } depth_stencil_block;

    struct BlendFactorBlock
    {
        VkBool32 blend_enable;
        VkBlendFactor src_colour_blend_factor;
        VkBlendFactor dst_colour_blend_factor;
        VkBlendOp colour_blend_op;
        VkBlendFactor src_alpha_blend_factor;
        VkBlendFactor dst_alpha_blend_factor;
        VkBlendOp alpha_blend_op;
    } blend_factor_block;

    VkPipelineLayout pl_layout;
    VkRenderPass render_pass;
    VkPipelineShaderStageCreateInfo shaders[RPE_BACKEND_SHADER_STAGE_MAX_COUNT];
    VkVertexInputAttributeDescription vert_attr_descs[VKAPI_PIPELINE_MAX_VERTEX_ATTR_COUNT];
    VkVertexInputBindingDescription vert_bind_descs[VKAPI_PIPELINE_MAX_INPUT_BIND_COUNT];
    VkSpecializationMapEntry spec_map_entries[RPE_BACKEND_SHADER_STAGE_MAX_COUNT]
                                             [VKAPI_PIPELINE_MAX_SPECIALIZATION_COUNT];
    uint32_t spec_map_entry_count[RPE_BACKEND_SHADER_STAGE_MAX_COUNT];
    // Note: The specialisation constant underlying data is hashed to ensure a new pipeline is
    // created and bound if this changes. WIthout this, only the pointer to the data is hashed into
    // the pipeline key, which isn't satisfactory as this rarely (if ever) changes.
    uint32_t spec_data_hash[RPE_BACKEND_SHADER_STAGE_MAX_COUNT];
    uint32_t tesse_vert_count;
    uint32_t colour_attach_count;
})
graphics_pl_key_t;

typedef struct ComputePipelineKey
{
    VkPipelineShaderStageCreateInfo shader;
    VkPipelineLayout pl_layout;
} compute_pl_key_t;

struct RenderPassState
{
    VkRenderPass instance;
    uint32_t colour_attach_count;
};

typedef struct PipelineCache
{
    vkapi_driver_t* driver;

    hash_set_t gfx_pipelines;
    hash_set_t compute_pipelines;
    hash_set_t pipeline_layouts;

    /// current bound pipeline.
    graphics_pl_key_t bound_graphics_pline;
    compute_pl_key_t bound_compute_pline;

    /// the requirements of the current descriptor and pipelines
    graphics_pl_key_t graphics_pline_requires;
    compute_pl_key_t compute_pline_requires;

    struct RenderPassState rpass_state;

} vkapi_pipeline_cache_t;

vkapi_pipeline_cache_t* vkapi_pline_cache_init(arena_t* arena, vkapi_driver_t* driver);

void vkapi_pline_cache_bind_graphics_pline(
    vkapi_pipeline_cache_t* c,
    VkCommandBuffer cmds,
    struct SpecConstParams* spec_consts,
    bool force_rebind);

vkapi_graphics_pl_t* vkapi_pline_cache_find_or_create_gfx_pline(
    vkapi_pipeline_cache_t* c, struct SpecConstParams* spec_consts);

void vkapi_pline_cache_bind_compute_pipeline(vkapi_pipeline_cache_t* c, VkCommandBuffer cmd_buffer);

vkapi_compute_pl_t* vkapi_pline_cache_find_or_create_compute_pline(vkapi_pipeline_cache_t* c);

void vkapi_pline_cache_bind_gfx_pl_layout(vkapi_pipeline_cache_t* c, VkPipelineLayout layout);
void vkapi_pline_cache_bind_compute_pl_layout(vkapi_pipeline_cache_t* c, VkPipelineLayout layout);

void vkapi_pline_cache_bind_gfx_shader_modules(vkapi_pipeline_cache_t* c, shader_prog_bundle_t* b);
void vkapi_pline_cache_bind_rpass(vkapi_pipeline_cache_t* c, VkRenderPass rpass);
void vkapi_pline_cache_bind_cull_mode(vkapi_pipeline_cache_t* c, VkCullModeFlagBits cullmode);
void vkapi_pline_cache_bind_polygon_mode(vkapi_pipeline_cache_t* c, VkPolygonMode polymode);
void vkapi_pline_cache_bind_front_face(vkapi_pipeline_cache_t* c, VkFrontFace face);
void vkapi_pline_cache_bind_topology(vkapi_pipeline_cache_t* c, VkPrimitiveTopology topo);
void vkapi_pline_cache_bind_prim_restart(vkapi_pipeline_cache_t* c, bool state);
void vkapi_pline_cache_bind_depth_stencil_block(
    vkapi_pipeline_cache_t* c, struct DepthStencilBlock* ds);
void vkapi_pline_cache_bind_colour_attach_count(vkapi_pipeline_cache_t* c, uint32_t count);
void vkapi_pline_cache_bind_tess_vert_count(vkapi_pipeline_cache_t* c, size_t count);
void vkapi_pline_cache_bind_blend_factor_block(
    vkapi_pipeline_cache_t* c, struct BlendFactorBlock* state);
void vkapi_pline_cache_bind_depth_test_enable(vkapi_pipeline_cache_t* c, bool state);
void vkapi_pline_cache_bind_depth_write_enable(vkapi_pipeline_cache_t* c, bool state);
void vkapi_pline_cache_bind_depth_compare_op(vkapi_pipeline_cache_t* c, VkCompareOp op);
void vkapi_pline_cache_bind_depth_clamp(vkapi_pipeline_cache_t* c, bool state);

void vkapi_pline_cache_bind_spec_constants(vkapi_pipeline_cache_t* c, shader_prog_bundle_t* b);

void vkapi_pline_cache_bind_vertex_input(
    vkapi_pipeline_cache_t* c,
    VkVertexInputAttributeDescription* vert_attr_descs,
    VkVertexInputBindingDescription* vert_bind_descs);

vkapi_pl_layout_t*
vkapi_pline_cache_get_pl_layout(vkapi_pipeline_cache_t* c, shader_prog_bundle_t* bundle);

void vkapi_pline_cache_bind_compute_shader_modules(
    vkapi_pipeline_cache_t* c, shader_prog_bundle_t* b);

void vkapi_pline_cache_destroy(vkapi_pipeline_cache_t* c);
void vkapi_pline_cache_gc(vkapi_pipeline_cache_t* c, uint64_t current_frame);

bool vkapi_pline_cache_compare_graphic_keys(graphics_pl_key_t* lhs, graphics_pl_key_t* rhs);
bool vkapi_pline_cache_compare_compute_keys(compute_pl_key_t* lhs, compute_pl_key_t* rhs);
#endif