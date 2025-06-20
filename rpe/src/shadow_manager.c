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

#include "shadow_manager.h"

#include "camera.h"
#include "engine.h"
#include "managers/light_manager.h"
#include "managers/transform_manager.h"
#include "material.h"
#include "scene.h"

#include <tracy/TracyC.h>
#include <utility/arena.h>

rpe_shadow_manager_t* rpe_shadow_manager_init(rpe_engine_t* engine, struct ShadowSettings settings)
{
    assert(settings.cascade_count <= RPE_SHADOW_MANAGER_MAX_CASCADE_COUNT);

    arena_t* arena = &engine->perm_arena;
    rpe_shadow_manager_t* sm = ARENA_MAKE_ZERO_STRUCT(arena, rpe_shadow_manager_t);
    sm->settings = settings;

    vkapi_driver_t* driver = engine->driver;

    sm->csm_shaders[RPE_BACKEND_SHADER_STAGE_VERTEX] = program_cache_from_spirv(
        driver->prog_manager,
        driver->context,
        "shadow.vert.spv",
        RPE_BACKEND_SHADER_STAGE_VERTEX,
        arena);
    sm->csm_shaders[RPE_BACKEND_SHADER_STAGE_FRAGMENT] = program_cache_from_spirv(
        driver->prog_manager,
        driver->context,
        "shadow.frag.spv",
        RPE_BACKEND_SHADER_STAGE_FRAGMENT,
        arena);

    if ((!vkapi_is_valid_shader_handle(sm->csm_shaders[RPE_BACKEND_SHADER_STAGE_VERTEX]) ||
         (!vkapi_is_valid_shader_handle(sm->csm_shaders[RPE_BACKEND_SHADER_STAGE_FRAGMENT]))))
    {
        return NULL;
    }

    sm->csm_bundle = program_cache_create_program_bundle(driver->prog_manager, arena);

    shader_bundle_update_descs_from_reflection(
        sm->csm_bundle, driver, sm->csm_shaders[RPE_BACKEND_SHADER_STAGE_VERTEX], arena);
    shader_bundle_update_descs_from_reflection(
        sm->csm_bundle, driver, sm->csm_shaders[RPE_BACKEND_SHADER_STAGE_FRAGMENT], arena);

    shader_bundle_set_depth_read_write_state(
        sm->csm_bundle, true, true, RPE_COMPARE_OP_LESS_OR_EQUAL);
    shader_bundle_set_depth_clamp_state(sm->csm_bundle, true);
    shader_bundle_set_cull_mode(sm->csm_bundle, RPE_CULL_MODE_FRONT);

    // Using the same layout as the material shaders though not all elements required for shadow.
    shader_bundle_add_vertex_input_binding(
        sm->csm_bundle,
        sm->csm_shaders[RPE_BACKEND_SHADER_STAGE_VERTEX],
        driver,
        0, // First location
        7, // End location.
        0, // Binding id.
        VK_VERTEX_INPUT_RATE_VERTEX);
    shader_bundle_add_vertex_input_binding(
        sm->csm_bundle,
        sm->csm_shaders[RPE_BACKEND_SHADER_STAGE_VERTEX],
        driver,
        8, // First location.
        9, // End location.
        1, // Binding id.
        VK_VERTEX_INPUT_RATE_INSTANCE);

    // SSBO buffer for cascade view-proj matrices
    sm->cascade_ubo = vkapi_res_cache_create_ssbo(
        driver->res_cache,
        driver,
        sizeof(struct CascadeInfo) * RPE_SHADOW_MANAGER_MAX_CASCADE_COUNT,
        0,
        VKAPI_BUFFER_HOST_TO_GPU);

    // The light manager has some dependencies on the shadow manager. We expect the light manager
    // to be initialised first - so update the light manager with any shadow dependency info.
    assert(engine->light_manager);
    rpe_light_manager_set_shadow_ssbo(engine->light_manager, sm->cascade_ubo);

    // Bind the SSBO to their positions in the shader.
    shader_bundle_update_ssbo_desc(
        sm->csm_bundle,
        RPE_SHADOW_MANAGER_CASCADE_VP_SSBO_BINDING,
        sm->cascade_ubo,
        RPE_SHADOW_MANAGER_MAX_CASCADE_COUNT);
    shader_bundle_update_ssbo_desc(
        sm->csm_bundle,
        RPE_SHADOW_MANAGER_TRANSFORM_SSBO_BINDING,
        engine->transform_manager->transform_buffer_handle,
        RPE_SCENE_MAX_STATIC_MODEL_COUNT);

    if (settings.enable_debug_cascade)
    {
        sm->csm_debug_shaders[RPE_BACKEND_SHADER_STAGE_VERTEX] = program_cache_from_spirv(
            driver->prog_manager,
            driver->context,
            "fullscreen_quad.vert.spv",
            RPE_BACKEND_SHADER_STAGE_VERTEX,
            &engine->perm_arena);
        sm->csm_debug_shaders[RPE_BACKEND_SHADER_STAGE_FRAGMENT] = program_cache_from_spirv(
            driver->prog_manager,
            driver->context,
            "shadow_cascade_debug.frag.spv",
            RPE_BACKEND_SHADER_STAGE_FRAGMENT,
            &engine->perm_arena);

        sm->csm_debug_bundle = program_cache_create_program_bundle(driver->prog_manager, arena);

        shader_bundle_update_descs_from_reflection(
            sm->csm_debug_bundle,
            driver,
            sm->csm_debug_shaders[RPE_BACKEND_SHADER_STAGE_VERTEX],
            arena);
        shader_bundle_update_descs_from_reflection(
            sm->csm_debug_bundle,
            driver,
            sm->csm_debug_shaders[RPE_BACKEND_SHADER_STAGE_FRAGMENT],
            arena);

        sm->csm_debug_bundle->raster_state.cull_mode = VK_CULL_MODE_FRONT_BIT;
        sm->csm_debug_bundle->raster_state.front_face = VK_FRONT_FACE_CLOCKWISE;
    }

    return sm;
}

void rpe_shadow_manager_update_draw_buffer(rpe_shadow_manager_t* sm, rpe_scene_t* scene)
{
    assert(sm);
    assert(scene);
    // The draw data buffer is updated at a later stage as the scene isn't available
    // at the point the shadow manager is initialised.
    shader_bundle_update_ssbo_desc(
        sm->csm_bundle,
        RPE_SHADOW_MANAGER_DRAW_DATA_SSBO_BINDING,
        scene->draw_data_handle,
        RPE_SCENE_MAX_STATIC_MODEL_COUNT);
}

void rpe_shadow_manager_compute_csm_splits(
    rpe_shadow_manager_t* m, rpe_scene_t* scene, rpe_camera_t* camera)
{
    TracyCZoneN(ctx, "SM::CsmSplits", 1);

    float clip_range = camera->z - camera->n;
    float min_z = camera->n;
    float max_z = camera->n + clip_range;
    float ratio = max_z / min_z;
    size_t cascade_count = m->settings.cascade_count;

    for (size_t i = 0; i < cascade_count; ++i)
    {
        float ci = (float)(i + 1) / (float)cascade_count;
        float uniform = min_z + (max_z - min_z) * ci;
        float log_c = min_z * powf(ratio, ci);
        float C = m->settings.split_lambda * (log_c - uniform) + uniform;
        scene->cascade_offsets[i] = (C - min_z) / clip_range;
    }

    TracyCZoneEnd(ctx);
}

void update_projections_runner(void* data)
{
    assert(data);
    struct JobEntry* je = (struct JobEntry*)data;

    rpe_scene_t* scene = je->scene;
    rpe_camera_t* camera = je->camera;
    rpe_shadow_manager_t* sm = je->sm;
    int idx = je->idx;

    // ================ Update the cascade shadow maps =========================
    // Adapted from: https://alextardif.com/shadowmapping.html

    float last_split = idx == 0 ? 0.0f : scene->cascade_offsets[idx - 1];
    float clip_range = camera->z - camera->n;

    math_mat4f inv_vp = math_mat4f_inverse(math_mat4f_mul(camera->projection, camera->view));

    math_vec3f corners[8] = {
        {-1.0f, 1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {1.0f, -1.0f, 0.0f},
        {-1.0f, -1.0f, 0.0f},
        {-1.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
        {1.0f, -1.0f, 1.0f},
        {-1.0f, -1.0f, 1.0f},
    };

    // Transform each corner to camera space using the inverse view-proj matrix.
    for (int j = 0; j < 8; ++j)
    {
        math_vec4f f = math_mat4f_mul_vec(inv_vp, math_vec4f_init_vec3(corners[j], 1.0f));
        corners[j].x = f.x / f.w;
        corners[j].y = f.y / f.w;
        corners[j].z = f.z / f.w;
    }

    // Adjust frustum corners based on current split distance.
    for (int j = 0; j < 4; ++j)
    {
        math_vec3f dist = math_vec3f_sub(corners[j + 4], corners[j]);
        corners[j + 4] =
            math_vec3f_add(corners[j], math_vec3f_mul_sca(dist, scene->cascade_offsets[idx]));
        corners[j] = math_vec3f_add(corners[j], math_vec3f_mul_sca(dist, last_split));
    }

    // Find the center of the frustrum.
    math_vec3f center = {0};
    for (int j = 0; j < 8; ++j)
    {
        center = math_vec3f_add(corners[j], center);
    }
    center = math_vec3f_mul_sca(center, 1.0f / 8.0f);

    // Create a consistent projection size by creating a circle around the frustum and
    // projecting over that - this reduces shimmering.
    float radius = math_vec3f_distance(corners[0], corners[6]) * 0.5f;
    float texels_per_unit = (float)sm->settings.cascade_dims / radius;

    math_mat4f scalar_mat = math_mat4f_identity();
    math_mat4f_scale(
        math_vec3f_init(texels_per_unit, texels_per_unit, texels_per_unit), &scalar_mat);

    // Get the directional light params.
    assert(je->dir_light);
    math_vec3f light_dir = math_vec3f_normalise(math_vec3f_mul_sca(je->dir_light->position, -1.0f));

    // Create the look-at matrix from the perspective of the light and scale it.
    math_vec3f up = {0.0f, 1.0f, 0.0f};
    math_vec3f zero = {0.0f, 0.0f, 0.0f};
    math_mat4f light_lookat = math_mat4f_lookat(light_dir, zero, up);
    light_lookat = math_mat4f_mul(scalar_mat, light_lookat);
    math_mat4f inv_lookat = math_mat4f_inverse(light_lookat);

    math_vec4f t_center = math_mat4f_mul_vec(light_lookat, math_vec4f_init_vec3(center, 1.0f));

    // Clamp to texel increment.
    t_center.x = floorf(t_center.x);
    t_center.y = floorf(t_center.y);
    // Convert back to original space.
    t_center = math_mat4f_mul_vec(inv_lookat, t_center);

    center.x = t_center.x / t_center.w;
    center.y = t_center.y / t_center.w;
    center.z = t_center.z / t_center.w;

    math_vec3f eye = math_vec3f_sub(center, math_vec3f_mul_sca(light_dir, -radius));

    // View matrix looking at the texel-corrected frustum center from the directional light
    // source.
    math_mat4f light_view = math_mat4f_lookat(center, eye, up);
    math_mat4f light_ortho =
        math_mat4f_ortho(-radius, radius, -radius, radius, -radius * 6.0f, radius * 6.0f);

    scene->shadow_map.cascades[idx].vp = math_mat4f_mul(light_ortho, light_view);
    scene->shadow_map.cascades[idx].split_depth =
        (camera->n + scene->cascade_offsets[idx] * clip_range) * -1.0f;
}

void rpe_shadow_manager_update_projections(
    rpe_shadow_manager_t* sm,
    rpe_camera_t* camera,
    rpe_scene_t* scene,
    rpe_engine_t* engine,
    rpe_light_manager_t* lm)
{
    job_queue_t* jq = engine->job_queue;
    sm->parent_job = job_queue_create_parent_job(engine->job_queue);

    for (int i = 0; i < sm->settings.cascade_count; ++i)
    {
        struct JobEntry* entry = &sm->job_entries[i];
        entry->sm = sm;
        entry->scene = scene;
        entry->camera = camera;
        entry->idx = i;
        entry->dir_light = rpe_light_manager_get_dir_light_params(lm);

        entry->job = job_queue_create_job(jq, update_projections_runner, entry, sm->parent_job);
        job_queue_run_job(jq, sm->job_entries[i].job);
    }
}

void rpe_shadow_manager_sync_update(rpe_shadow_manager_t* m, rpe_engine_t* engine)
{
    job_queue_run_and_wait(engine->job_queue, m->parent_job);
}

void rpe_shadow_manager_upload_projections(
    rpe_shadow_manager_t* m, rpe_engine_t* engine, rpe_scene_t* scene)
{
    vkapi_driver_map_gpu_buffer(
        engine->driver,
        m->cascade_ubo,
        m->settings.cascade_count * sizeof(struct CascadeInfo),
        0,
        scene->shadow_map.cascades);
}

// Public functions

void rpe_shadow_manager_update(
    rpe_shadow_manager_t* sm, rpe_scene_t* scene, struct ShadowSettings* settings)
{
    assert(sm);
    assert(scene);
    assert(scene->curr_camera);
    sm->settings = *settings;
    rpe_shadow_manager_compute_csm_splits(sm, scene, scene->curr_camera);
}