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

#include "pipeline_cache.h"

#include "buffer.h"
#include "descriptor_cache.h"
#include "driver.h"
#include "program_manager.h"
#include "shader.h"
#include "texture.h"

#include <string.h>
#include <utility/arena.h>
#include <utility/hash.h>

graphics_pl_key_t vkapi_graphics_pline_key_init()
{
    graphics_pl_key_t k = {0};

    k.raster_state.cull_mode = VK_CULL_MODE_BACK_BIT;
    k.raster_state.polygon_mode = VK_POLYGON_MODE_FILL;
    k.raster_state.front_face = VK_FRONT_FACE_CLOCKWISE;
    k.raster_state.prim_restart = VK_FALSE;
    k.raster_state.depth_test_enable = VK_FALSE;
    k.raster_state.depth_write_enable = VK_FALSE;
    k.raster_state.colour_write_mask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    k.blend_factor_block.src_colour_blend_factor = VK_BLEND_FACTOR_ZERO;
    k.blend_factor_block.dst_colour_blend_factor = VK_BLEND_FACTOR_ZERO;
    k.blend_factor_block.colour_blend_op = VK_BLEND_OP_ADD;
    k.blend_factor_block.src_alpha_blend_factor = VK_BLEND_FACTOR_ZERO;
    k.blend_factor_block.dst_alpha_blend_factor = VK_BLEND_FACTOR_ZERO;
    k.blend_factor_block.alpha_blend_op = VK_BLEND_OP_ADD;
    k.blend_factor_block.blend_enable = VK_FALSE;

    k.depth_stencil_block.stencil_test_enable = VK_FALSE;
    k.depth_stencil_block.compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
    k.depth_stencil_block.depth_fail_op = VK_STENCIL_OP_ZERO;
    k.depth_stencil_block.pass_op = VK_STENCIL_OP_ZERO;
    k.depth_stencil_block.compare_mask = 0;
    k.depth_stencil_block.write_mask = 0;
    k.depth_stencil_block.reference = 0;

    return k;
}

vkapi_pipeline_cache_t* vkapi_pline_cache_init(arena_t* arena, vkapi_driver_t* driver)
{
    vkapi_pipeline_cache_t* c = ARENA_MAKE_ZERO_STRUCT(arena, vkapi_pipeline_cache_t);
    c->graphics_pline_requires = vkapi_graphics_pline_key_init();
    c->gfx_pipelines = HASH_SET_CREATE(graphics_pl_key_t, vkapi_graphics_pl_t, arena);
    c->compute_pipelines = HASH_SET_CREATE(compute_pl_key_t, vkapi_compute_pl_t, arena);
    c->pipeline_layouts = HASH_SET_CREATE(pl_layout_key_t, vkapi_pl_layout_t, arena);
    c->driver = driver;

    return c;
}

bool vkapi_pline_cache_compare_graphic_keys(graphics_pl_key_t* lhs, graphics_pl_key_t* rhs)
{
    return memcmp(lhs, rhs, sizeof(graphics_pl_key_t)) == 0; // NOLINT
}

bool vkapi_pline_cache_compare_compute_keys(compute_pl_key_t* lhs, compute_pl_key_t* rhs)
{
    return memcmp(lhs, rhs, sizeof(compute_pl_key_t)) == 0; // NOLINT
}

void vkapi_pline_cache_bind_graphics_pline(
    vkapi_pipeline_cache_t* c,
    VkCommandBuffer cmds,
    struct SpecConstParams* spec_consts,
    bool force_rebind)
{
    assert(c);

    // Set the renderpass separate from the key - that's because the key is cleared on each bind
    // call, but when re-using the same renderpass for different draw-calls but rebinding pipeline,
    // as the renderpass is set via being_rpass, it will be cleared with no way of getting the
    // previous renderpass.
    assert(c->rpass_state.instance && "[PipelineCahce] No render pass has been declared.");
    c->graphics_pline_requires.render_pass = c->rpass_state.instance;
    c->graphics_pline_requires.colour_attach_count = c->rpass_state.colour_attach_count;

    // Check if the required pipeline is already bound. If so, nothing to do
    // here.
    if (vkapi_pline_cache_compare_graphic_keys(
            &c->graphics_pline_requires, &c->bound_graphics_pline) &&
        !force_rebind)
    {
        vkapi_graphics_pl_t* pl = HASH_SET_GET(&c->gfx_pipelines, &c->bound_graphics_pline);
        assert(pl);
        pl->last_used_frame_stamp = c->driver->current_frame;
        c->graphics_pline_requires = vkapi_graphics_pline_key_init();
        return;
    }

    vkapi_graphics_pl_t* pl = vkapi_pline_cache_find_or_create_gfx_pline(c, spec_consts);
    assert(pl);
    pl->last_used_frame_stamp = c->driver->current_frame;
    vkCmdBindPipeline(cmds, VK_PIPELINE_BIND_POINT_GRAPHICS, pl->instance);

    c->bound_graphics_pline = c->graphics_pline_requires;
    c->graphics_pline_requires = vkapi_graphics_pline_key_init();
}

vkapi_graphics_pl_t* vkapi_pline_cache_find_or_create_gfx_pline(
    vkapi_pipeline_cache_t* c, struct SpecConstParams* spec_consts)
{
    assert(c);
    assert(c->graphics_pline_requires.pl_layout);
    vkapi_graphics_pl_t* pl = HASH_SET_GET(&c->gfx_pipelines, &c->graphics_pline_requires);
    if (pl)
    {
        return pl;
    }

    vkapi_graphics_pl_t new_pl =
        vkapi_graph_pl_create(c->driver->context, &c->graphics_pline_requires, spec_consts);
    return HASH_SET_INSERT(&c->gfx_pipelines, &c->graphics_pline_requires, &new_pl);
}

void vkapi_pline_cache_bind_compute_pipeline(vkapi_pipeline_cache_t* c, VkCommandBuffer cmd_buffer)
{
    assert(c);

    vkapi_compute_pl_t* pline = vkapi_pline_cache_find_or_create_compute_pline(c);
    assert(pline);

    vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pline->instance);
    c->bound_compute_pline = c->compute_pline_requires;
}

vkapi_compute_pl_t* vkapi_pline_cache_find_or_create_compute_pline(vkapi_pipeline_cache_t* c)
{
    assert(c);
    vkapi_compute_pl_t* pl = HASH_SET_GET(&c->compute_pipelines, &c->compute_pline_requires);
    if (pl)
    {
        return pl;
    }
    vkapi_compute_pl_t new_pl =
        vkapi_compute_pl_create(c->driver->context, &c->compute_pline_requires);
    return HASH_SET_INSERT(&c->compute_pipelines, &c->compute_pline_requires, &new_pl);
}

void vkapi_pline_cache_bind_gfx_shader_modules(vkapi_pipeline_cache_t* c, shader_prog_bundle_t* b)
{
    shader_bundle_get_shader_stage_create_info_all(
        b, c->driver, c->graphics_pline_requires.shaders);
}

void vkapi_pline_cache_bind_compute_shader_modules(
    vkapi_pipeline_cache_t* c, shader_prog_bundle_t* b)
{
    c->compute_pline_requires.shader =
        shader_bundle_get_shader_stage_create_info(b, c->driver, RPE_BACKEND_SHADER_STAGE_COMPUTE);
}

void vkapi_pline_cache_bind_rpass(vkapi_pipeline_cache_t* c, VkRenderPass rpass)
{
    assert(c);
    assert(rpass);
    c->rpass_state.instance = rpass;
}

void vkapi_pline_cache_bind_gfx_pl_layout(vkapi_pipeline_cache_t* c, VkPipelineLayout layout)
{
    assert(c);
    c->graphics_pline_requires.pl_layout = layout;
}

void vkapi_pline_cache_bind_compute_pl_layout(vkapi_pipeline_cache_t* c, VkPipelineLayout layout)
{
    assert(c);
    c->compute_pline_requires.pl_layout = layout;
}

void vkapi_pline_cache_bind_cull_mode(vkapi_pipeline_cache_t* c, VkCullModeFlagBits cullmode)
{
    assert(c);
    c->graphics_pline_requires.raster_state.cull_mode = cullmode;
}

void vkapi_pline_cache_bind_polygon_mode(vkapi_pipeline_cache_t* c, VkPolygonMode polymode)
{
    assert(c);
    c->graphics_pline_requires.raster_state.polygon_mode = polymode;
}

void vkapi_pline_cache_bind_front_face(vkapi_pipeline_cache_t* c, VkFrontFace face)
{
    assert(c);
    c->graphics_pline_requires.raster_state.front_face = face;
}

void vkapi_pline_cache_bind_topology(vkapi_pipeline_cache_t* c, VkPrimitiveTopology topo)
{
    assert(c);
    c->graphics_pline_requires.raster_state.topology = topo;
}

void vkapi_pline_cache_bind_prim_restart(vkapi_pipeline_cache_t* c, bool state)
{
    assert(c);
    c->graphics_pline_requires.raster_state.prim_restart = state;
}

void vkapi_pline_cache_bind_depth_stencil_block(
    vkapi_pipeline_cache_t* c, struct DepthStencilBlock* ds)
{
    assert(c);
    c->graphics_pline_requires.depth_stencil_block = *ds;
}

void vkapi_pline_cache_bind_depth_test_enable(vkapi_pipeline_cache_t* c, bool state)
{
    assert(c);
    c->graphics_pline_requires.raster_state.depth_test_enable = state;
}

void vkapi_pline_cache_bind_depth_write_enable(vkapi_pipeline_cache_t* c, bool state)
{
    assert(c);
    c->graphics_pline_requires.raster_state.depth_write_enable = state;
}

void vkapi_pline_cache_bind_depth_compare_op(vkapi_pipeline_cache_t* c, VkCompareOp op)
{
    assert(c);
    c->graphics_pline_requires.raster_state.depth_compare_op = op;
}

void vkapi_pline_cache_bind_depth_clamp(vkapi_pipeline_cache_t* c, bool state)
{
    assert(c);
    c->graphics_pline_requires.raster_state.depth_clamp_enable = state;
}

void vkapi_pline_cache_bind_colour_attach_count(vkapi_pipeline_cache_t* c, uint32_t count)
{
    assert(c);
    c->rpass_state.colour_attach_count = count;
}

void vkapi_pline_cache_bind_tess_vert_count(vkapi_pipeline_cache_t* c, size_t count)
{
    assert(c);
    c->graphics_pline_requires.tesse_vert_count = count;
}

void vkapi_pline_cache_bind_blend_factor_block(
    vkapi_pipeline_cache_t* c, struct BlendFactorBlock* state)
{
    assert(c);
    c->graphics_pline_requires.blend_factor_block = *state;
}

void vkapi_pline_cache_bind_spec_constants(vkapi_pipeline_cache_t* c, shader_prog_bundle_t* b)
{
    assert(c);
    assert(b);
    for (int i = 0; i < RPE_BACKEND_SHADER_STAGE_MAX_COUNT; ++i)
    {
        if (b->spec_const_params[i].entry_count > 0)
        {
            memcpy(
                c->graphics_pline_requires.spec_map_entries[i],
                b->spec_const_params[i].entries,
                sizeof(VkSpecializationMapEntry) * b->spec_const_params[i].entry_count);
            c->graphics_pline_requires.spec_map_entry_count[i] =
                b->spec_const_params[i].entry_count;
            c->graphics_pline_requires.spec_data_hash[i] =
                murmur2_hash(b->spec_const_params[i].data, b->spec_const_params[i].data_size, 0);
        }
    }
}

void vkapi_pline_cache_bind_vertex_input(
    vkapi_pipeline_cache_t* c,
    VkVertexInputAttributeDescription* vert_attr_descs,
    VkVertexInputBindingDescription* vert_bind_descs)
{
    assert(c);
    assert(vert_attr_descs);
    assert(vert_bind_descs);
    memcpy(
        c->graphics_pline_requires.vert_attr_descs,
        vert_attr_descs,
        sizeof(VkVertexInputAttributeDescription) * VKAPI_PIPELINE_MAX_VERTEX_ATTR_COUNT);
    memcpy(
        c->graphics_pline_requires.vert_bind_descs,
        vert_bind_descs,
        sizeof(VkVertexInputBindingDescription) * VKAPI_PIPELINE_MAX_INPUT_BIND_COUNT);
}

vkapi_pl_layout_t*
vkapi_pline_cache_get_pl_layout(vkapi_pipeline_cache_t* c, shader_prog_bundle_t* bundle)
{
    assert(c);
    assert(bundle);

    // We create layouts for all supported sets even if they are not used - so its safe here
    // just to check if the first layout has been created as the rest will be NULL also (i hope!!)
    if (bundle->desc_layouts[0] == VK_NULL_HANDLE)
    {
        vkapi_desc_cache_create_pl_layouts(c->driver, bundle);
    }

    // Create the key and check if a pipeline layout fits the criteria in the cache.
    pl_layout_key_t key = {0};
    memcpy(
        key.layouts,
        bundle->desc_layouts,
        sizeof(VkDescriptorSetLayout) * VKAPI_PIPELINE_MAX_DESC_SET_COUNT);
    for (int i = 0; i < RPE_BACKEND_SHADER_STAGE_MAX_COUNT; ++i)
    {
        key.push_block_info[i].size = bundle->push_blocks[i].range;
        key.push_block_info[i].stage = bundle->push_blocks[i].stage;
    }

    vkapi_pl_layout_t* l = HASH_SET_GET(&c->pipeline_layouts, &key);
    if (l)
    {
        l->frame_last_used = c->driver->current_frame;
        return l;
    }

    // Not in the cache, create a new instance then.
    vkapi_pl_layout_t new_l = {0};

    VkPushConstantRange constant_ranges[RPE_BACKEND_SHADER_STAGE_MAX_COUNT];
    uint32_t cr_count = 0;

    for (int i = 0; i < RPE_BACKEND_SHADER_STAGE_MAX_COUNT; ++i)
    {
        if (bundle->push_blocks[i].range > 0)
        {
            VkPushConstantRange pcr = {
                .size = bundle->push_blocks[i].range,
                .offset = 0,
                .stageFlags = bundle->push_blocks[i].stage};
            constant_ranges[cr_count++] = pcr;
        }
    }

    VkPipelineLayoutCreateInfo pl_info = {0};
    pl_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pl_info.pushConstantRangeCount = cr_count;
    pl_info.pPushConstantRanges = constant_ranges;
    pl_info.setLayoutCount = VKAPI_PIPELINE_MAX_DESC_SET_COUNT;
    pl_info.pSetLayouts = bundle->desc_layouts;

    VK_CHECK_RESULT(vkCreatePipelineLayout(
        c->driver->context->device, &pl_info, VK_NULL_HANDLE, &new_l.instance));

    new_l.frame_last_used = c->driver->current_frame;
    return HASH_SET_INSERT(&c->pipeline_layouts, &key, &new_l);
}

void vkapi_pline_cache_gc(vkapi_pipeline_cache_t* c, uint64_t current_frame)
{
    assert(c);
    // Destroy any pipelines that have reached there lifetime after their last use.
    hash_set_iterator_t it = hash_set_iter_create(&c->gfx_pipelines);
    for (;;)
    {
        vkapi_graphics_pl_t* pl = hash_set_iter_next(&it);
        if (!pl)
        {
            break;
        }

        uint64_t collection_frame = pl->last_used_frame_stamp + VKAPI_PIPELINE_LIFETIME_FRAME_COUNT;
        if (collection_frame < current_frame)
        {
            vkDestroyPipeline(c->driver->context->device, pl->instance, VK_NULL_HANDLE);
            it = hash_set_iter_erase(&it);
        }
    }
}

void vkapi_pline_cache_destroy(vkapi_pipeline_cache_t* c)
{
    assert(c);

    hash_set_iterator_t it = hash_set_iter_create(&c->pipeline_layouts);
    for (;;)
    {
        vkapi_pl_layout_t* pl = hash_set_iter_next(&it);
        if (!pl)
        {
            break;
        }
        if (pl->instance)
        {
            vkDestroyPipelineLayout(c->driver->context->device, pl->instance, VK_NULL_HANDLE);
        }
    }
    hash_set_clear(&c->pipeline_layouts);

    // Destroy all graphics pipelines associated with this cache.
    it = hash_set_iter_create(&c->gfx_pipelines);
    for (;;)
    {
        vkapi_graphics_pl_t* pl = hash_set_iter_next(&it);
        if (!pl)
        {
            break;
        }

        if (pl->instance)
        {
            vkDestroyPipeline(c->driver->context->device, pl->instance, VK_NULL_HANDLE);
        }
    }
    hash_set_clear(&c->gfx_pipelines);

    // Destroy all compute pipelines associated with this cache.
    it = hash_set_iter_create(&c->compute_pipelines);
    for (;;)
    {
        vkapi_compute_pl_t* pl = hash_set_iter_next(&it);
        if (!pl)
        {
            break;
        }

        if (pl->instance)
        {
            vkDestroyPipeline(c->driver->context->device, pl->instance, VK_NULL_HANDLE);
        }
    }
    hash_set_clear(&c->compute_pipelines);
}
