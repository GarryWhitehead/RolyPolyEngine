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

#ifndef __PRIV_SHADOW_MANAGER_H__
#define __PRIV_SHADOW_MANAGER_H__

#include "rpe/settings.h"

#include <stdint.h>
#include <utility/job_queue.h>
#include <utility/maths.h>
#include <vulkan-api/program_manager.h>
#include <vulkan-api/resource_cache.h>

#define RPE_SHADOW_MANAGER_MAX_CASCADE_COUNT 8
#define RPE_SHADOW_MANAGER_CASCADE_VP_SSBO_BINDING 0
#define RPE_SHADOW_MANAGER_TRANSFORM_SSBO_BINDING 1
#define RPE_SHADOW_MANAGER_DRAW_DATA_SSBO_BINDING 2

typedef struct Engine rpe_engine_t;
typedef struct Scene rpe_scene_t;
typedef struct Camera rpe_camera_t;
typedef struct LightManager rpe_light_manager_t;
typedef struct ShadowManager rpe_shadow_manager_t;
typedef struct LightInstance rpe_light_instance_t;

/**
 Entry data for projection update jobs.
 */
struct JobEntry
{
    rpe_shadow_manager_t* sm;
    rpe_camera_t* camera;
    rpe_scene_t* scene;
    job_t* job;
    rpe_light_instance_t* dir_light;
    int idx;
};

typedef struct ShadowMap
{
    struct CascadeInfo
    {
        math_mat4f vp;
        float split_depth;
    } cascades[RPE_SHADOW_MANAGER_MAX_CASCADE_COUNT];

} rpe_shadow_map;

typedef struct ShadowManager
{
    struct ShadowSettings settings;

    // ================= vulkan backend =======================

    shader_prog_bundle_t* csm_bundle;
    // Only valid if debugging enabled.
    shader_prog_bundle_t* csm_debug_bundle;
    shader_handle_t csm_shaders[2];
    shader_handle_t csm_debug_shaders[2];
    buffer_handle_t cascade_ubo;
    job_t* parent_job;
    struct JobEntry job_entries[RPE_SHADOW_MANAGER_MAX_CASCADE_COUNT];
} rpe_shadow_manager_t;

rpe_shadow_manager_t* rpe_shadow_manager_init(rpe_engine_t* engine, struct ShadowSettings settings);

void rpe_shadow_manager_update_projections(
    rpe_shadow_manager_t* m,
    rpe_camera_t* camera,
    rpe_scene_t* scene,
    rpe_engine_t* engine,
    rpe_light_manager_t* lm);

// Check that the projection update jobs have finished running.
void rpe_shadow_manager_sync_update(rpe_shadow_manager_t* m, rpe_engine_t* engine);

// Make sure this is called after updating the projections (see above).
void rpe_shadow_manager_upload_projections(
    rpe_shadow_manager_t* m, rpe_engine_t* engine, rpe_scene_t* scene);

void rpe_shadow_manager_compute_csm_splits(
    rpe_shadow_manager_t* m, rpe_scene_t* scene, rpe_camera_t* camera);

void rpe_shadow_manager_update_draw_buffer(rpe_shadow_manager_t* sm, rpe_scene_t* scene);


#endif