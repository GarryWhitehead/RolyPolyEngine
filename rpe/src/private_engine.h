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

#ifndef __RPE_PRIVATE_ENGINE_H__
#define __RPE_PRIVATE_ENGINE_H__

#include "rpe/engine.h"
#include "vulkan-api/swapchain.h"

#define RPE_ENGINE_MAX_SWAPCHAIN_COUNT 4
#define RPE_ENGINE_SCRATCH_ARENA_SIZE 1 << 15
#define RPE_ENGINE_PERM_ARENA_SIZE 1 << 30

// Forward declarations.
typedef struct VkApiDriver vkapi_driver_t;
typedef struct VkApiSwapchain vkapi_swapchain_t;

struct SwapchainHandle
{
    /// Index into the swap chain array.
    uint32_t idx;
};

struct Engine
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
};

#endif
