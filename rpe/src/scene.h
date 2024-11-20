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

#ifndef __RPE_SCENE_H__
#define __RPE_SCENE_H__

#include "aabox.h"
#include "render_queue.h"

#include <stdbool.h>
#include <utility/arena.h>
#include <utility/maths.h>
#include <vulkan-api/resource_cache.h>

#define RPE_SCENE_MAX_STATIC_MODEL_COUNT 3000
#define RPE_SCENE_MAX_SKINNED_MODEL_COUNT 3000

typedef struct Renderable rpe_renderable_t;
typedef struct Engine rpe_engine_t;
typedef struct TransformParams rpe_transform_params_t;
typedef struct Compute rpe_compute_t;
typedef struct Camera rpe_camera_t;

typedef struct RenderableExtents
{
    math_vec3f center;
    float pad0;
    math_vec3f extent;
    float pad1;
} rpe_rend_extents_t;

typedef struct Scene
{
    rpe_render_queue_t render_queue;
    arena_dyn_array_t objects;

    math_mat4f* skinned_transforms;
    math_mat4f* static_transforms;
    buffer_handle_t skinned_buffer_handle;
    buffer_handle_t static_buffer_handle;

    // Visibility culling checks.
    rpe_rend_extents_t* rend_extents;
    buffer_handle_t extents_buffer;
    buffer_handle_t vis_status_buffer;
    rpe_compute_t* cull_compute;

    // Current camera information
    rpe_camera_t* curr_camera;
    buffer_handle_t cam_ubo;

} rpe_scene_t;

rpe_scene_t* rpe_scene_init(rpe_engine_t* engine, arena_t* arena);

bool rpe_scene_update(rpe_scene_t* scene, rpe_engine_t* engine);

#endif