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

#ifndef __RPE_SCENE_H__
#define __RPE_SCENE_H__

#include "rpe/aabox.h"

#include <stdbool.h>
#include <utility/arena.h>
#include <utility/maths.h>
#include <vulkan-api/resource_cache.h>

#define RPE_SCENE_MAX_STATIC_MODEL_COUNT 3000
#define RPE_SCENE_MAX_BONE_COUNT 1000
#define RPE_SCENE_CAMERA_UBO_BINDING 0
#define RPE_SCENE_SKIN_SSBO_BINDING 0
#define RPE_SCENE_TRANSFORM_SSBO_BINDING 1
#define RPE_SCENE_DRAW_DATA_SSBO_BINDING 2

typedef struct Renderable rpe_renderable_t;
typedef struct Engine rpe_engine_t;
typedef struct TransformParams rpe_transform_params_t;
typedef struct Compute rpe_compute_t;
typedef struct Camera rpe_camera_t;
typedef struct RenderQueue rpe_render_queue_t;
typedef struct RenderableManager rpe_rend_manager_t;
typedef struct TransformManager rpe_transform_manager_t;
typedef struct Ibl ibl_t;

struct DrawData;

typedef struct RenderableExtents
{
    math_vec4f center;
    math_vec4f extent;
} rpe_rend_extents_t;

typedef struct SceneUbo
{
    uint32_t model_count;
    uint32_t ibl_mip_levels;
} rpe_scene_ubo_t;

typedef struct Scene
{
    rpe_render_queue_t* render_queue;
    arena_dyn_array_t objects;

    // Used on the fragment shader - data from each material instance.
    struct DrawData* draw_data;
    buffer_handle_t draw_data_handle;

    rpe_rend_extents_t* rend_extents;
    buffer_handle_t extents_buffer;

    // Mesh data used to create the VkIndirectDraw object array on the compute (Host->GPU)
    buffer_handle_t mesh_data_handle;
    // The indirect draw cmds based upon culling by the compute (GPU only)
    buffer_handle_t indirect_draw_handle;
    // Model draw data - indices based upon culling by the compute shader (GPU only).
    buffer_handle_t model_draw_data_handle;
    // Draw count for each batch (GPU only).
    buffer_handle_t draw_count_handle;
    // Total draw count buffer (GPU only)
    buffer_handle_t total_draw_handle;
    rpe_compute_t* cull_compute;

    // Current camera information
    rpe_camera_t* curr_camera;
    // Current IBL instance (optional).
    ibl_t* curr_ibl;

    buffer_handle_t scene_ubo;

} rpe_scene_t;

rpe_scene_t* rpe_scene_init(rpe_engine_t* engine, arena_t* arena);

bool rpe_scene_update(rpe_scene_t* scene, rpe_engine_t* engine);

void rpe_scene_upload_extents(
    rpe_scene_t* scene, rpe_engine_t* engine, rpe_rend_manager_t* rm, rpe_transform_manager_t* tm);

/** Public functions **/

rpe_scene_t* rpe_scene_create(rpe_engine_t* engine);

#endif