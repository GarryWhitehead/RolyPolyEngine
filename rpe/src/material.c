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

#include "material.h"

#include "engine.h"
#include "managers/renderable_manager.h"
#include "render_queue.h"

#include <utility/arena.h>
#include <vulkan-api/program_manager.h>
#include <vulkan-api/sampler_cache.h>
#include <vulkan-api/utility.h>

rpe_material_t* rpe_material_init(rpe_engine_t* e, arena_t* arena)
{
    assert(e);

    rpe_material_t* instance = ARENA_MAKE_STRUCT(arena, rpe_material_t, ARENA_ZERO_MEMORY);
    instance->view_layer = 0x2;
    instance->double_sided = false;
    MAKE_DYN_ARRAY(struct BufferInfo, arena, 50, &instance->buffers);
    instance->program_bundle = program_cache_create_program_bundle(e->driver->prog_manager, arena);

    shader_bundle_update_spec_const_data(
        instance->program_bundle,
        sizeof(struct MeshConstants),
        &instance->mesh_consts,
        RPE_BACKEND_SHADER_STAGE_VERTEX);
    shader_bundle_update_spec_const_data(
        instance->program_bundle,
        sizeof(struct MaterialConstants),
        &instance->material_consts,
        RPE_BACKEND_SHADER_STAGE_FRAGMENT);

    shader_bundle_update_descs_from_reflection(
        instance->program_bundle,
        e->driver,
        e->mat_shaders[RPE_BACKEND_SHADER_STAGE_VERTEX],
        arena);
    shader_bundle_update_descs_from_reflection(
        instance->program_bundle,
        e->driver,
        e->mat_shaders[RPE_BACKEND_SHADER_STAGE_FRAGMENT],
        arena);

    return instance;
}

void rpe_material_add_variant(enum MaterialImageType type, rpe_material_t* m)
{
    switch (type)
    {
        case RPE_MATERIAL_IMAGE_TYPE_BASE_COLOR:
            m->material_consts.has_base_colour_sampler = true;
            break;
        case RPE_MATERIAL_IMAGE_TYPE_NORMAL:
            m->material_consts.has_normal_sampler = true;
            break;
        case RPE_MATERIAL_IMAGE_TYPE_METALLIC_ROUGHNESS:
            m->material_consts.has_mr_sampler = true;
            break;
        case RPE_MATERIAL_IMAGE_TYPE_EMISSIVE:
            m->material_consts.has_emissive_sampler = true;
            break;
        case RPE_MATERIAL_IMAGE_TYPE_OCCLUSION:
            m->material_consts.has_occlusion_sampler = true;
            break;
        default:
            log_warn("Invalid material variant bit. Ignoring....");
    }
}

void rpe_material_set_pipeline(rpe_material_t* m, enum MaterialPipeline pipeline)
{
    if (pipeline == RPE_MATERIAL_PIPELINE_MR)
    {
        m->material_consts.pipeline_type = RPE_MATERIAL_PIPELINE_MR;
    }
    else if (pipeline == RPE_MATERIAL_PIPELINE_SPECULAR)
    {
        m->material_consts.pipeline_type = RPE_MATERIAL_PIPELINE_SPECULAR;
    }
}

void rpe_material_add_image_texture(
    rpe_material_t* m,
    vkapi_driver_t* driver,
    rpe_mapped_texture_t* tex,
    enum MaterialImageType type,
    enum ShaderStage stage,
    sampler_params_t params,
    uint32_t binding)
{
    assert(binding < VKAPI_PIPELINE_MAX_SAMPLER_BIND_COUNT);
    rpe_material_add_variant(type, m);

    params.mip_levels = tex->mip_levels;
    VkSampler* sampler = vkapi_sampler_cache_create(driver->sampler_cache, &params, driver);
    shader_bundle_add_texture_sampler(m->program_bundle, *sampler, binding);
}

void rpe_material_add_buffer(rpe_material_t* m, buffer_handle_t handle, enum ShaderStage stage)
{
    assert(m);
    struct BufferInfo i = {.handle = handle, i.stage = stage};
    DYN_ARRAY_APPEND(&m->buffers, &i);
}

void rpe_material_set_render_prim(rpe_material_t* m, rpe_render_primitive_t* p)
{
    // add the render primitive, with sub meshes (not properly implemented yet);
    shader_bundle_add_render_primitive(m->program_bundle, p->topology, p->prim_restart);
}

void rpe_material_set_blend_factors(rpe_material_t* m, struct MaterialBlendFactor factors)
{
    assert(m);
    m->program_bundle->blend_state.colour = factors.colour;
}

void rpe_material_set_double_sided_state(rpe_material_t* m, bool state)
{
    assert(m);
    m->double_sided = state;
    m->program_bundle->raster_state.cull_mode = state ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
}

void rpe_material_set_test_enable(rpe_material_t* m, bool state)
{
    assert(m);
    m->program_bundle->ds_state.test_enable = state;
}

void rpe_material_set_write_enable(rpe_material_t* m, bool state)
{
    assert(m);
    m->program_bundle->ds_state.write_enable = state;
}

void rpe_material_set_depth_compare_op(rpe_material_t* m, VkCompareOp op)
{
    assert(m);
    m->program_bundle->ds_state.compare_op = op;
}

void rpe_material_set_polygon_mode(rpe_material_t* m, VkPolygonMode mode)
{
    assert(m);
    m->program_bundle->raster_state.polygon_mode = mode;
}

void rpe_material_set_front_face(rpe_material_t* m, VkFrontFace face)
{
    assert(m);
    m->program_bundle->raster_state.front_face = face;
}

void rpe_material_set_cull_mode(rpe_material_t* m, VkCullModeFlagBits mode)
{
    assert(m);
    m->program_bundle->raster_state.cull_mode = mode;
}

void rpe_material_set_scissor(
    rpe_material_t* m, uint32_t width, uint32_t height, uint32_t xOffset, uint32_t yOffset)
{
    assert(m);
    shader_bundle_set_scissor(m->program_bundle, width, height, xOffset, yOffset);
}

void rpe_material_set_viewport(
    rpe_material_t* m, uint32_t width, uint32_t height, float minDepth, float maxDepth)
{
    assert(m);
    shader_bundle_set_viewport(m->program_bundle, width, height, minDepth, maxDepth);
}

void rpe_material_set_view_layer(rpe_material_t* m, uint8_t layer)
{
    assert(m);
    if (layer > RPE_RENDER_QUEUE_MAX_VIEW_LAYER_COUNT)
    {
        log_warn(
            "Layer value of %i is outside max allowed value (%i). Ignoring.",
            layer,
            RPE_RENDER_QUEUE_MAX_VIEW_LAYER_COUNT);
        return;
    }
    m->view_layer = layer;
}

void rpe_material_set_base_colour_factor(rpe_material_t* m, math_vec4f* f)
{
    assert(m);
    m->material_draw_data.base_colour_factor = *f;
    m->material_consts.has_base_colour_factor = true;
}

void rpe_material_set_diffuse_factor(rpe_material_t* m, math_vec4f* f)
{
    assert(m);
    m->material_draw_data.diffuse_factor = *f;
    m->material_consts.has_diffuse_factor = true;
}

void rpe_material_set_bas_colour_factor(rpe_material_t* m, math_vec4f* f)
{
    assert(m);
    m->material_draw_data.specular_factor = *f;
}

void rpe_material_set_emissive_factor(rpe_material_t* m, math_vec4f* f)
{
    assert(m);
    m->material_draw_data.base_colour_factor = *f;
    m->material_consts.has_emissive_factor = true;
}

void rpe_material_set_roughness_factor(rpe_material_t* m, float f)
{
    assert(m);
    m->material_draw_data.roughness_factor = f;
}

void rpe_material_set_metallic_factor(rpe_material_t* m, float f)
{
    assert(m);
    m->material_draw_data.metallic_factor = f;
}

void rpe_material_set_alpha_mask(rpe_material_t* m, float mask)
{
    assert(m);
    m->material_draw_data.alpha_mask = mask;
    m->material_consts.has_alpha_mask = true;
}

void rpe_material_set_alpha_cutoff(rpe_material_t* m, float co)
{
    assert(m);
    m->material_draw_data.alpha_mask_cut_off = co;
    m->material_consts.has_alpha_mask_cutoff = true;
}
