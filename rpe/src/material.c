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
#include "managers/transform_manager.h"
#include "render_queue.h"
#include "scene.h"

#include <backend/convert_to_vk.h>
#include <utility/arena.h>
#include <vulkan-api/program_manager.h>
#include <vulkan-api/sampler_cache.h>
#include <vulkan-api/utility.h>

rpe_material_t rpe_material_init(rpe_engine_t* e, arena_t* arena)
{
    assert(e);

    rpe_material_t instance = {0};
    instance.view_layer = 0x2;
    instance.double_sided = false;
    MAKE_DYN_ARRAY(struct BufferInfo, arena, 50, &instance.buffers);
    instance.program_bundle = program_cache_create_program_bundle(e->driver->prog_manager, arena);

    shader_bundle_update_descs_from_reflection(
        instance.program_bundle, e->driver, e->mat_shaders[RPE_BACKEND_SHADER_STAGE_VERTEX], arena);
    shader_bundle_update_descs_from_reflection(
        instance.program_bundle,
        e->driver,
        e->mat_shaders[RPE_BACKEND_SHADER_STAGE_FRAGMENT],
        arena);

    shader_bundle_add_vertex_input_binding(
        instance.program_bundle,
        e->mat_shaders[RPE_BACKEND_SHADER_STAGE_VERTEX],
        e->driver,
        0, // First location
        5, // End location.
        0, // Binding id.
        VK_VERTEX_INPUT_RATE_VERTEX);
    shader_bundle_add_vertex_input_binding(
        instance.program_bundle,
        e->mat_shaders[RPE_BACKEND_SHADER_STAGE_VERTEX],
        e->driver,
        6, // First location.
        7, // End location.
        1, // Binding id.
        VK_VERTEX_INPUT_RATE_INSTANCE);

    shader_bundle_update_ubo_desc(
        instance.program_bundle, RPE_SCENE_CAMERA_UBO_BINDING, e->curr_scene->cam_ubo);
    shader_bundle_update_ssbo_desc(
        instance.program_bundle,
        RPE_SCENE_SKIN_SSBO_BINDING,
        e->transform_manager->bone_buffer_handle,
        RPE_SCENE_MAX_BONE_COUNT);
    shader_bundle_update_ssbo_desc(
        instance.program_bundle,
        RPE_SCENE_TRANSFORM_SSBO_BINDING,
        e->transform_manager->transform_buffer_handle,
        RPE_SCENE_MAX_STATIC_MODEL_COUNT);
    shader_bundle_update_ssbo_desc(
        instance.program_bundle,
        RPE_SCENE_DRAW_DATA_SSBO_BINDING,
        e->curr_scene->draw_data_handle,
        RPE_SCENE_MAX_STATIC_MODEL_COUNT);

    return instance;
}

void rpe_material_add_variant(enum MaterialImageType type, rpe_material_t* m)
{
    switch (type)
    {
        case RPE_MATERIAL_IMAGE_TYPE_BASE_COLOR:
            m->material_consts.has_base_colour_sampler = 1;
            break;
        case RPE_MATERIAL_IMAGE_TYPE_NORMAL:
            m->material_consts.has_normal_sampler = 1;
            break;
        case RPE_MATERIAL_IMAGE_TYPE_METALLIC_ROUGHNESS:
            m->material_consts.has_mr_sampler = 1;
            break;
        case RPE_MATERIAL_IMAGE_TYPE_EMISSIVE:
            m->material_consts.has_emissive_sampler = 1;
            break;
        case RPE_MATERIAL_IMAGE_TYPE_OCCLUSION:
            m->material_consts.has_occlusion_sampler = 1;
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

void rpe_material_update_vertex_constants(rpe_material_t* mat, rpe_mesh_t* mesh)
{
    // A position vertex is mandatory.
    assert(mesh->mesh_flags & RPE_MESH_ATTRIBUTE_POSITION);
    if (mesh->mesh_flags & RPE_MESH_ATTRIBUTE_NORMAL)
    {
        mat->mesh_consts.has_normal = 1;
        mat->material_consts.has_normal = 1;
    }
    if (mesh->mesh_flags & RPE_MESH_ATTRIBUTE_UV)
    {
        mat->material_consts.has_uv = 1;
    }
    if (mesh->mesh_flags & RPE_MESH_ATTRIBUTE_COLOUR)
    {
        mat->material_consts.has_colour_attr = 1;
    }
    if ((mesh->mesh_flags & RPE_MESH_ATTRIBUTE_BONE_ID) &&
        (mesh->mesh_flags & RPE_MESH_ATTRIBUTE_BONE_WEIGHT))
    {
        mat->mesh_consts.has_skin = 1;
    }

    shader_bundle_update_spec_const_data(
        mat->program_bundle,
        sizeof(struct MeshConstants),
        &mat->mesh_consts,
        RPE_BACKEND_SHADER_STAGE_VERTEX);
    shader_bundle_update_spec_const_data(
        mat->program_bundle,
        sizeof(struct MaterialConstants),
        &mat->material_consts,
        RPE_BACKEND_SHADER_STAGE_FRAGMENT);
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

void rpe_material_set_blend_factors(rpe_material_t* m, struct MaterialBlendFactor factors)
{
    assert(m);
    m->program_bundle->blend_state.colour = factors.colour;
    m->material_key.blend_state = factors;
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
    m->material_key.depth_test_enable = state;
}

void rpe_material_set_write_enable(rpe_material_t* m, bool state)
{
    assert(m);
    m->program_bundle->ds_state.write_enable = state;
    m->material_key.depth_write_enable = state;
}

void rpe_material_set_depth_compare_op(rpe_material_t* m, enum CompareOp op)
{
    assert(m);
    m->program_bundle->ds_state.compare_op = compare_op_to_vk(op);
    m->material_key.depth_compare_op = op;
}

void rpe_material_set_polygon_mode(rpe_material_t* m, enum PolygonMode mode)
{
    assert(m);
    m->program_bundle->raster_state.polygon_mode = polygon_mode_to_vk(mode);
    m->material_key.polygon_mode = mode;
}

void rpe_material_set_front_face(rpe_material_t* m, enum FrontFace face)
{
    assert(m);
    m->program_bundle->raster_state.front_face = front_face_to_vk(face);
    m->material_key.front_face = face;
}

void rpe_material_set_cull_mode(rpe_material_t* m, enum CullMode mode)
{
    assert(m);
    m->program_bundle->raster_state.cull_mode = cull_mode_to_vk(mode);
    m->material_key.cull_mode = mode;
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

void rpe_material_set_specular_factor(rpe_material_t* m, math_vec4f* f)
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
