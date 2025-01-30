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

#include "vulkan-api/driver.h"

#include <utility/arena.h>

typedef struct Engine rpe_engine_t;
typedef struct Scene rpe_scene_t;
typedef struct SwapchainHandle swapchain_handle_t;
typedef struct ObjectManager rpe_obj_manager_t;
typedef struct Renderer rpe_renderer_t;
typedef struct RenderableManager rpe_rend_manager_t;
typedef struct TransformManager rpe_transform_manager_t;
typedef struct Renderable rpe_renderable_t;
typedef struct Material rpe_material_t;
typedef struct Mesh rpe_mesh_t;

/**
 Create a new engine instance.
 @param driver A pointer to a vulkan driver. This must have been initialised before calling this
 function.
 @return An initialised engine opaque pointer.
 */
rpe_engine_t* rpe_engine_create(vkapi_driver_t* driver);

/**
 Close down all resources used by the specified engine.
 @param engine A engine instance.
 */
void rpe_engine_shutdown(rpe_engine_t* engine);

/**
 Create a new swapchain - required for rendering to a window.
 @param engine A pointer to the engine.
 @param surface A Vulkan surface opaque pointer, this is unique to the window which this swapchain
 will be associated with.
 @param width The width of the window in pixels.
 @param height The height of the window in pixels.
 @return A handle to the swapchain.
 */
swapchain_handle_t* rpe_engine_create_swapchain(
    rpe_engine_t* engine, VkSurfaceKHR surface, uint32_t width, uint32_t height);

rpe_renderer_t* rpe_engine_create_renderer(rpe_engine_t* engine);
rpe_renderable_t*
rpe_engine_create_renderable(rpe_engine_t* engine, rpe_material_t* mat, rpe_mesh_t* mesh);

void rpe_engine_set_current_scene(rpe_engine_t* engine, rpe_scene_t* scene);
void rpe_engine_set_current_swapchain(rpe_engine_t* engine, swapchain_handle_t* handle);

rpe_rend_manager_t* rpe_engine_get_rend_manager(rpe_engine_t* engine);
rpe_obj_manager_t* rpe_engine_get_obj_manager(rpe_engine_t* engine);
rpe_transform_manager_t* rpe_engine_get_transform_manager(rpe_engine_t* engine);


#endif