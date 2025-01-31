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

#ifndef __VKAPI_SWAPCHAIN_H__
#define __VKAPI_SWAPCHAIN_H__

#include "common.h"
#include "resource_cache.h"

#define VKAPI_SWAPCHAIN_MAX_IMAGE_COUNT 3

// Forward declarations.
typedef struct VkApiContext vkapi_context_t;

typedef struct SwapchainContext
{
    texture_handle_t handle;

} vkapi_swapchain_context_t;

typedef struct VkApiSwapchain
{
    // the dimensions of the current swapchain
    VkExtent2D extent;
    // a swapchain based on the present surface type
    VkSwapchainKHR sc_instance;
    VkSurfaceFormatKHR surface_format;
    vkapi_swapchain_context_t contexts[VKAPI_SWAPCHAIN_MAX_IMAGE_COUNT];
    uint32_t image_count;
} vkapi_swapchain_t;


vkapi_swapchain_t vkapi_swapchain_init();

void vkapi_swapchain_destroy(vkapi_driver_t* driver, vkapi_swapchain_t* swapchain);

int vkapi_swapchain_create(
    vkapi_driver_t* driver,
    vkapi_swapchain_t* swapchain,
    VkSurfaceKHR surface,
    uint32_t win_width,
    uint32_t win_height,
    arena_t* scratch_arena);

/// creates the image views for the swapchain
void vkapi_swapchain_prepare_views(
    vkapi_driver_t* driver, vkapi_swapchain_t* swapchain, arena_t* scratch_arena);

#endif