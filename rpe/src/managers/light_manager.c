/* Copyright (c) 2024-2025 Garry Whitehead
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

#include "light_manager.h"

#include "camera.h"
#include "engine.h"
#include "rpe/light_manager.h"
#include "scene.h"
#include "shadow_manager.h"

#include <string.h>
#include <utility/arena.h>
#include <vulkan-api/driver.h>
#include <vulkan-api/sampler_cache.h>

rpe_light_manager_t* rpe_light_manager_init(rpe_engine_t* engine, arena_t* arena)
{
    rpe_light_manager_t* lm = ARENA_MAKE_ZERO_STRUCT(arena, rpe_light_manager_t);
    vkapi_driver_t* driver = engine->driver;
    MAKE_DYN_ARRAY(struct LightInstance, arena, 50, &lm->lights);

    lm->ssbo_vk_buffer_handle = vkapi_res_cache_create_ssbo(
        driver->res_cache,
        driver,
        sizeof(struct LightSsbo) * RPE_LIGHTING_SAMPLER_MAX_LIGHT_COUNT,
        0,
        VKAPI_BUFFER_HOST_TO_GPU);

    lm->shaders[RPE_BACKEND_SHADER_STAGE_VERTEX] = program_cache_from_spirv(
        driver->prog_manager,
        driver->context,
        "fullscreen_quad.vert.spv",
        RPE_BACKEND_SHADER_STAGE_VERTEX,
        &engine->perm_arena);
    lm->shaders[RPE_BACKEND_SHADER_STAGE_FRAGMENT] = program_cache_from_spirv(
        driver->prog_manager,
        driver->context,
        "lighting.frag.spv",
        RPE_BACKEND_SHADER_STAGE_FRAGMENT,
        &engine->perm_arena);

    if ((!vkapi_is_valid_shader_handle(lm->shaders[RPE_BACKEND_SHADER_STAGE_VERTEX]) ||
         (!vkapi_is_valid_shader_handle(lm->shaders[RPE_BACKEND_SHADER_STAGE_FRAGMENT]))))
    {
        return NULL;
    }

    lm->program_bundle = program_cache_create_program_bundle(driver->prog_manager, arena);

    shader_bundle_update_descs_from_reflection(
        lm->program_bundle, driver, lm->shaders[RPE_BACKEND_SHADER_STAGE_VERTEX], arena);
    shader_bundle_update_descs_from_reflection(
        lm->program_bundle, driver, lm->shaders[RPE_BACKEND_SHADER_STAGE_FRAGMENT], arena);

    shader_bundle_update_spec_const_data(
        lm->program_bundle,
        sizeof(struct LightingConstants),
        &lm->light_consts,
        RPE_BACKEND_SHADER_STAGE_FRAGMENT);

    // Binding for the camera UBO
    shader_bundle_update_ubo_desc(
        lm->program_bundle, RPE_LIGHT_MANAGER_CAMERA_UBO_BINDING, engine->camera_ubo);

    lm->dir_light_obj.id = UINT32_MAX;
    lm->engine = engine;
    lm->comp_manager = rpe_comp_manager_init(arena);
    lm->program_bundle->raster_state.cull_mode = VK_CULL_MODE_FRONT_BIT;
    lm->program_bundle->raster_state.front_face = VK_FRONT_FACE_CLOCKWISE;
    return lm;
}

void rpe_light_manager_set_shadow_ssbo(rpe_light_manager_t* lm , buffer_handle_t cascade_ubo)
{
    shader_bundle_update_ssbo_desc(
        lm->program_bundle,
        RPE_LIGHT_MANAGER_SHADOW_CASCADE_SSBO_BINDING,
        cascade_ubo,
        RPE_SHADOW_MANAGER_MAX_CASCADE_COUNT);
}

void rpe_light_manager_calculate_spot_cone(
    rpe_light_instance_t* light, float outerCone, float innerCone)
{
    if (light->type != RPE_LIGHTING_TYPE_SPOT)
    {
        return;
    }

    // First calculate the spotlight cone values.
    float outer = MIN(fabsf(outerCone), (float)M_PI);
    float inner = MIN(fabsf(innerCone), (float)M_PI);
    inner = MIN(inner, outer);

    float cos_outer = cosf(outer);
    float cos_inner = cosf(inner);

    light->spot_light_info.outer = outer;
    light->spot_light_info.cos_outer_sq = cos_outer * cos_outer;
    light->spot_light_info.scale = 1.0f / MAX(1.0f / 1024.0f, cos_inner - cos_outer);
    light->spot_light_info.offset = -cos_outer * light->spot_light_info.scale;
}

void set_intensity(rpe_light_instance_t* light, float intensity, enum LightType type)
{
    switch (type)
    {
        case RPE_LIGHTING_TYPE_DIRECTIONAL:
            light->intensity = intensity;
            break;
        case RPE_LIGHTING_TYPE_POINT:
            light->intensity = intensity * (float)M_1_PI * 0.25f;
            break;
        case RPE_LIGHTING_TYPE_SPOT:
            light->intensity = intensity * (float)M_1_PI;
            break;
    }
}

void set_radius(rpe_light_instance_t* light, float fallout)
{
    if (light->type != RPE_LIGHTING_TYPE_DIRECTIONAL)
    {
        light->spot_light_info.radius = fallout;
    }
}

void set_sun_angular_radius(rpe_light_manager_t* lm, rpe_light_instance_t* light, float radius)
{
    if (light->type == RPE_LIGHTING_TYPE_DIRECTIONAL)
    {
        radius = CLAMP(radius, 0.25f, 20.0f);
        lm->sun_angular_radius = math_to_radians(radius);
    }
}

void set_sun_halo_size(rpe_light_manager_t* lm, rpe_light_instance_t* light, float size)
{
    if (light->type == RPE_LIGHTING_TYPE_DIRECTIONAL)
    {
        lm->sun_halo_size = size;
    }
}

void set_sun_halo_falloff(rpe_light_manager_t* lm, rpe_light_instance_t* light, float falloff)
{
    if (light->type == RPE_LIGHTING_TYPE_DIRECTIONAL)
    {
        lm->sun_halo_falloff = falloff;
    }
}

/** Public entry function **/
void rpe_light_manager_create_light(
    rpe_light_manager_t* lm, rpe_light_create_info_t* ci, rpe_object_t obj, enum LightType type)
{
    assert(lm);

    // First, add the object which will give us a free slot.
    uint64_t idx = rpe_comp_manager_add_obj(lm->comp_manager, obj);

    struct LightInstance instance = {
        .type = type,
        .position = ci->position,
        .target = ci->target,
        .colour = ci->colour,
        .fov = ci->fov,
        .spot_light_info.radius = ci->fallout};

    set_radius(&instance, ci->fallout);
    set_intensity(&instance, ci->intensity, type);
    rpe_light_manager_calculate_spot_cone(&instance, ci->outer_cone, ci->inner_cone);

    set_sun_angular_radius(lm, &instance, ci->sun_angular_radius);
    set_sun_halo_size(lm, &instance, ci->sun_halo_size);
    set_sun_halo_falloff(lm, &instance, ci->sun_halo_falloff);

    // keep track of the directional light as its parameters are needed
    // for rendering the sun.
    if (type == RPE_LIGHTING_TYPE_DIRECTIONAL)
    {
        lm->dir_light_obj = obj;
    }

    ADD_OBJECT_TO_MANAGER(&lm->lights, idx, &instance);
}

void rpe_light_manager_update(rpe_light_manager_t* lm, rpe_scene_t* scene, rpe_camera_t* camera)
{
    assert(lm);
    assert(scene);

    rpe_shadow_manager_t* sm = lm->engine->shadow_manager;

    lm->light_consts.has_ibl = scene->curr_ibl ? true : false;
    lm->light_consts.csm_split_count = sm->settings.cascade_count;

    // Set the scene UBO each update as the current scene may have changed (could instead just
    // update on a call to set_current_scene?)
    shader_bundle_update_ubo_desc(lm->program_bundle, 1, scene->scene_ubo);

    math_vec3f up = {0.0f, 1.0f, 0.0f};
    for (size_t i = 0; i < lm->lights.size; ++i)
    {
        struct LightInstance* light = DYN_ARRAY_GET_PTR(struct LightInstance, &lm->lights, i);
        math_mat4f projection = math_mat4f_projection(light->fov, 1.0f, camera->n, camera->z);
        light->mvp =
            math_mat4f_mul(projection, math_mat4f_lookat(light->target, light->position, up));
    }
}

void rpe_light_manager_update_ssbo(
    rpe_light_manager_t* lm, struct LightInstance* lights, uint32_t count)
{
    assert(lm);
    assert(count < RPE_LIGHTING_SAMPLER_MAX_LIGHT_COUNT);

    memset(
        (void*)lm->ssbo_buffers,
        0,
        sizeof(struct LightSsbo) * RPE_LIGHTING_SAMPLER_MAX_LIGHT_COUNT);

    int vis_count = 0;
    for (size_t i = 0; i < count; ++i)
    {
        struct LightInstance* light = &lights[i];
        if (!light->is_visible)
        {
            continue;
        }

        struct LightSsbo* buffer = &lm->ssbo_buffers[vis_count++];
        buffer->mvp = light->mvp;
        buffer->pos = math_vec4f_init_vec3(light->position, 1.0f);
        buffer->direction = math_vec4f_init_vec3(light->target, 1.0f);
        buffer->colour = math_vec4f_init_vec3(light->colour, light->intensity);
        buffer->type = light->type;
        buffer->fall_out = light->type != RPE_LIGHTING_TYPE_DIRECTIONAL ? light->intensity : 0.0f;

        if (light->type == RPE_LIGHTING_TYPE_SPOT)
        {
            buffer->scale = light->spot_light_info.scale;
            buffer->offset = light->spot_light_info.offset;
        }
    }
    // The end of the viable lights to render is signified on the shader
    // by a light type of 0xFF;
    lm->ssbo_buffers[vis_count].type = RPE_LIGHTING_SAMPLER_END_OF_BUFFER_SIGNAL;

    size_t mapped_size = vis_count + 1 * sizeof(struct LightSsbo);
    vkapi_driver_map_gpu_buffer(
        lm->engine->driver, lm->ssbo_vk_buffer_handle, mapped_size, 0, lm->ssbo_buffers);
}

rpe_light_instance_t* rpe_light_manager_get_dir_light_params(rpe_light_manager_t* lm)
{
    assert(lm);
    if (lm->dir_light_obj.id != UINT64_MAX)
    {
        return rpe_light_manager_get_light_instance(lm, lm->dir_light_obj);
    }
    return NULL;
}

rpe_light_instance_t*
rpe_light_manager_get_light_instance(rpe_light_manager_t* lm, rpe_object_t obj)
{
    assert(lm);
    assert(rpe_comp_manager_has_obj(lm->comp_manager, obj));
    uint64_t idx = rpe_comp_manager_get_obj_idx(lm->comp_manager, obj);
    return DYN_ARRAY_GET_PTR(rpe_light_instance_t, &lm->lights, idx);
}

void rpe_light_manager_set_intensity(rpe_light_manager_t* lm, rpe_object_t obj, float intensity)
{
    assert(lm);
    rpe_light_instance_t* i = rpe_light_manager_get_light_instance(lm, obj);
    set_intensity(i, intensity, i->type);
}

void rpe_light_manager_set_fallout(rpe_light_manager_t* lm, rpe_object_t obj, float fallout)
{
    assert(lm);
    rpe_light_instance_t* i = rpe_light_manager_get_light_instance(lm, obj);
    set_radius(i, fallout);
}

void rpe_light_manager_set_position(rpe_light_manager_t* lm, rpe_object_t obj, math_vec3f* pos)
{
    assert(lm);
    rpe_light_instance_t* i = rpe_light_manager_get_light_instance(lm, obj);
    i->position = *pos;
}

void rpe_light_manager_set_target(rpe_light_manager_t* lm, rpe_object_t obj, math_vec3f* target)
{
    assert(lm);
    rpe_light_instance_t* i = rpe_light_manager_get_light_instance(lm, obj);
    i->target = *target;
}

void rpe_light_manager_set_colour(rpe_light_manager_t* lm, rpe_object_t obj, math_vec3f* col)
{
    assert(lm);
    rpe_light_instance_t* i = rpe_light_manager_get_light_instance(lm, obj);
    i->colour = *col;
}

void rpe_light_manager_set_fov(rpe_light_manager_t* lm, rpe_object_t obj, float fov)
{
    assert(lm);
    rpe_light_instance_t* i = rpe_light_manager_get_light_instance(lm, obj);
    i->fov = fov;
}

void rpe_light_manager_destroy(rpe_light_manager_t* lm, rpe_object_t obj)
{
    assert(lm);
    bool res = rpe_comp_manager_remove(lm->comp_manager, obj);
    assert(res);
}
