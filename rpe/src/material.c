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

rpe_material_t rpe_material_init(rpe_engine_t* e, rpe_scene_t* scene, arena_t* arena)
{
    assert(e);

    rpe_material_t instance = {0};
    instance.double_sided = false;
    // Material will cast shadows by default.
    instance.shadow_caster = true;
    // Disable PBR pipeline by default.
    instance.material_consts.pipeline_type = RPE_MATERIAL_PIPELINE_NONE;
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
        7, // End location.
        0, // Binding id.
        VK_VERTEX_INPUT_RATE_VERTEX);
    shader_bundle_add_vertex_input_binding(
        instance.program_bundle,
        e->mat_shaders[RPE_BACKEND_SHADER_STAGE_VERTEX],
        e->driver,
        8, // First location.
        9, // End location.
        1, // Binding id.
        VK_VERTEX_INPUT_RATE_INSTANCE);

    shader_bundle_update_ubo_desc(
        instance.program_bundle, RPE_SCENE_CAMERA_UBO_BINDING, scene->camera_ubo);
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
        scene->draw_data_handle,
        RPE_SCENE_MAX_STATIC_MODEL_COUNT);

    return instance;
}

uint32_t rpe_material_max_mipmaps(uint32_t width, uint32_t height)
{
    assert(width > 0);
    assert(width == height);
    uint32_t p = MAX(width, height);
    return (uint32_t)floorf(log2f((float)p) + 1);
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
    else
    {
        m->material_consts.pipeline_type = RPE_MATERIAL_PIPELINE_NONE;
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
    if (mesh->mesh_flags & RPE_MESH_ATTRIBUTE_TANGENT)
    {
        mat->material_consts.has_tangent = 1;
    }
    if (mesh->mesh_flags & RPE_MESH_ATTRIBUTE_UV0 || mesh->mesh_flags & RPE_MESH_ATTRIBUTE_UV1)
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

void rpe_material_add_buffer(rpe_material_t* m, buffer_handle_t handle, enum ShaderStage stage)
{
    assert(m);
    struct BufferInfo i = {.handle = handle, i.stage = stage};
    DYN_ARRAY_APPEND(&m->buffers, &i);
}

void rpe_material_set_blend_factors(rpe_material_t* m, struct MaterialBlendFactor factors)
{
    assert(m);
    m->material_key.blend_state.alpha = factors.alpha;
    m->material_key.blend_state.colour = factors.colour;
    m->material_key.blend_state.dst_alpha = factors.dst_alpha;
    m->material_key.blend_state.dst_colour = factors.dst_colour;
    m->material_key.blend_state.src_alpha = factors.src_alpha;
    m->material_key.blend_state.src_colour = factors.src_colour;
    m->material_key.blend_state.state = factors.state;

    m->program_bundle->blend_state.colour = blend_op_to_vk(factors.colour);
    m->program_bundle->blend_state.alpha = blend_op_to_vk(factors.alpha);
    m->program_bundle->blend_state.dst_alpha = blend_factor_to_vk(factors.dst_alpha);
    m->program_bundle->blend_state.dst_colour = blend_factor_to_vk(factors.dst_colour);
    m->program_bundle->blend_state.src_alpha = blend_factor_to_vk(factors.src_alpha);
    m->program_bundle->blend_state.src_colour = blend_factor_to_vk(factors.src_colour);
    m->program_bundle->blend_state.blend_enable = factors.state;
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

void rpe_material_set_topology(rpe_material_t* m, enum PrimitiveTopology topo)
{
    assert(m);
    m->program_bundle->render_prim.topology = primitive_topology_to_vk(topo);
    m->material_key.topo = topo;
}

void rpe_material_set_cull_mode(rpe_material_t* m, enum CullMode mode)
{
    assert(m);
    m->program_bundle->raster_state.cull_mode = cull_mode_to_vk(mode);
    m->material_key.cull_mode = mode;
}

void rpe_material_set_shadow_caster_state(rpe_material_t* m, bool state)
{
    assert(m);
    m->shadow_caster = state;
}

void rpe_material_set_base_colour_factor(rpe_material_t* m, math_vec4f* f)
{
    assert(m);
    m->material_draw_data.base_colour_factor = *f;
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
    m->material_draw_data.emissive_factor = *f;
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

void rpe_material_set_blend_factor_preset(rpe_material_t* m, enum BlendFactorPresets preset)
{
    struct MaterialBlendFactor params = {0};

    if (preset == RPE_BLEND_FACTOR_PRESET_TRANSLUCENT)
    {
        params.src_colour = RPE_BLEND_FACTOR_SRC_ALPHA;
        params.dst_colour = RPE_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        params.colour = RPE_BLEND_OP_ADD;
        params.src_alpha = RPE_BLEND_FACTOR_SRC_ALPHA;
        params.dst_alpha = RPE_BLEND_FACTOR_ZERO;
        params.alpha = RPE_BLEND_OP_ADD;
        params.state = VK_TRUE;
        rpe_material_set_blend_factors(m, params);
    }
    else
    {
        log_warn("Unrecognised blend factor preset. Skipped.");
    }
}

void rpe_material_set_type(rpe_material_t* m, enum MaterialType type)
{
    assert(m);
    m->material_consts.material_type = type;
    m->mesh_consts.material_type = type;
    m->material_key.material_type = type;
}

void rpe_material_set_device_texture(
    rpe_material_t* m, texture_handle_t h, enum MaterialImageType type, uint32_t uv_index)
{
    m->material_draw_data.image_indices[type] = h.id - VKAPI_RES_CACHE_MAX_RESERVED_COUNT;
    m->material_draw_data.uv_indices[type] = uv_index;
    rpe_material_add_variant(type, m);
}

texture_handle_t rpe_material_map_texture(
    rpe_engine_t* engine,
    rpe_mapped_texture_t* tex,
    sampler_params_t* params,
    bool generate_mipmaps)
{
    assert(engine);
    assert(tex);
    assert(params);

    vkapi_driver_t* driver = engine->driver;

    tex->mip_levels =
        generate_mipmaps ? rpe_material_max_mipmaps(tex->width, tex->height) : tex->mip_levels;
    params->mip_levels = tex->mip_levels;

    texture_handle_t h = vkapi_res_cache_create_tex2d(
        driver->res_cache,
        driver->context,
        driver->vma_allocator,
        driver->sampler_cache,
        tex->format,
        tex->width,
        tex->height,
        tex->mip_levels,
        tex->array_count,
        tex->type,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        params);
    assert(vkapi_tex_handle_is_valid(h));

    vkapi_driver_map_gpu_texture(
        engine->driver, h, tex->image_data, tex->image_data_size, tex->offsets, generate_mipmaps);

    return h;
}
