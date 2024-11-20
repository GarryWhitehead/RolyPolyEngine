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

#define RPE_ENGINE_MAX_SWAPCHAIN_COUNT 4
#define RPE_ENGINE_SCRATCH_ARENA_SIZE 1 << 15
#define RPE_ENGINE_PERM_ARENA_SIZE 1 << 30
#define RPE_ENGINE_FRAME_ARENA_SIZE 1 << 30

#include "managers/object_manager.h"

#include <stdint.h>
#include <stdlib.h>
#include <utility/arena.h>
#include <vulkan-api/program_manager.h>
#include <vulkan-api/swapchain.h>

typedef struct VkApiDriver vkapi_driver_t;
typedef struct RenderableManager rpe_rend_manager_t;
typedef struct TransformManager rpe_transform_manager_t;
typedef struct Scene rpe_scene_t;

typedef struct SwapchainHandle
{
    /// Index into the swap chain array.
    uint32_t idx;
} swapchain_handle_t;

typedef struct Engine
{
    /// A Vulkan driver instance.
    vkapi_driver_t* driver;
    /// A cache array of swapchains.
    vkapi_swapchain_t swap_chains[RPE_ENGINE_MAX_SWAPCHAIN_COUNT];
    /// The current number of swapchains that have been created.
    uint32_t swap_chain_count;
    /// A scratch arena - used for function scope allocations.
    arena_t scratch_arena;
    /// A permanent arena - lasts the lifetime of the engine.
    arena_t perm_arena;
    /// Frame arena - scoped for the length of a frame.
    arena_t frame_arena;

    vkapi_swapchain_t* curr_swapchain;
    rpe_scene_t* curr_scene;

    rpe_obj_manager_t obj_manager;
    rpe_rend_manager_t* rend_manager;
    rpe_transform_manager_t* transform_manager;

    // Material shader handles for each stage.
    shader_handle_t mat_shaders[RPE_BACKEND_SHADER_STAGE_MAX_COUNT];

} rpe_engine_t;

#endif