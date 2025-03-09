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
#ifndef __RPE_ENGINE_H__
#define __RPE_ENGINE_H__

#include "camera.h"
#include "vulkan-api/driver.h"

#include <utility/arena.h>

typedef struct Engine rpe_engine_t;
typedef struct Scene rpe_scene_t;
typedef struct SwapchainHandle swapchain_handle_t;
typedef struct ObjectManager rpe_obj_manager_t;
typedef struct Renderer rpe_renderer_t;
typedef struct RenderableManager rpe_rend_manager_t;
typedef struct TransformManager rpe_transform_manager_t;
typedef struct LightManager rpe_light_manager_t;
typedef struct Renderable rpe_renderable_t;
typedef struct Material rpe_material_t;
typedef struct Mesh rpe_mesh_t;
typedef struct Camera rpe_camera_t;
typedef struct Skybox rpe_skybox_t;
typedef struct Settings rpe_settings_t;
typedef struct JobQueue job_queue_t;

rpe_engine_t* rpe_engine_create(vkapi_driver_t* driver, rpe_settings_t* settings);

void rpe_engine_shutdown(rpe_engine_t* engine);

swapchain_handle_t* rpe_engine_create_swapchain(
    rpe_engine_t* engine, VkSurfaceKHR surface, uint32_t width, uint32_t height);

rpe_renderer_t* rpe_engine_create_renderer(rpe_engine_t* engine);
rpe_renderable_t*
rpe_engine_create_renderable(rpe_engine_t* engine, rpe_material_t* mat, rpe_mesh_t* mesh);
rpe_scene_t* rpe_engine_create_scene(rpe_engine_t* engine);
rpe_camera_t* rpe_engine_create_camera(
    rpe_engine_t* engine, float fovy, uint32_t width, uint32_t height, float n, float f, enum ProjectionType type);
rpe_skybox_t* rpe_engine_create_skybox(rpe_engine_t* engine);

void rpe_engine_set_current_scene(rpe_engine_t* engine, rpe_scene_t* scene);
void rpe_engine_set_current_swapchain(rpe_engine_t* engine, swapchain_handle_t* handle);

bool rpe_engine_destroy_scene(rpe_engine_t* engine, rpe_scene_t* scene);
bool rpe_engine_destroy_camera(rpe_engine_t* engine, rpe_camera_t* camera);
bool rpe_engine_destroy_renderer(rpe_engine_t* engine, rpe_renderer_t* renderer);

rpe_rend_manager_t* rpe_engine_get_rend_manager(rpe_engine_t* engine);
rpe_obj_manager_t* rpe_engine_get_obj_manager(rpe_engine_t* engine);
rpe_transform_manager_t* rpe_engine_get_transform_manager(rpe_engine_t* engine);
rpe_light_manager_t* rpe_engine_get_light_manager(rpe_engine_t* engine);

job_queue_t* rpe_engine_get_job_queue(rpe_engine_t* engine);

#endif