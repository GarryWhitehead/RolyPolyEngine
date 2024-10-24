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

#ifndef __VKAPI_DRIVER_H__
#define __VKAPI_DRIVER_H__

#include "commands.h"
#include "common.h"
#include "context.h"
#include "renderpass.h"
#include "resource_cache.h"
#include "staging_pool.h"

#define VKAPI_SCRATCH_ARENA_SIZE 1 << 20
#define VKAPI_PERM_ARENA_SIZE 1 << 20

// Forward declarations.
typedef struct VkApiResourceCache vkapi_res_cache_t;
typedef struct VkApiDriver vkapi_driver_t;

/**
 initialise the Vulkan driver - includes creating the abstract device,
 physical device, queues, etc.
 @param driver A pointer to the driver instance.
 @param surface An opaque pointer to a window surface. If NULL, then headless mode is assumed..
 @returns an error code specifying whether device creation was successful.
 */
int vkapi_driver_create_device(vkapi_driver_t* driver, VkSurfaceKHR surface);

/**
 Create a new driver instance - creates a Vulkan instance for this device.
 @param driver A pointer to the driver instance.
 @param instance_ext Vulkan instance extensions parsed from GLFW.
 @param ext_count The instance extension count.
 @param [out] driver The new initialised driver instance.
 @returns The error code returned by the call. If not VKAPI_SUCCESS, then the driver instance will
 be uninitialised.
 */
vkapi_driver_t* vkapi_driver_init(const char** instance_ext, uint32_t ext_count, int* errror_code);

/**
 Deallocate all resources associated with the vulkan api.
 @param driver A pointer to the driver instance.
 */
void vkapi_driver_shutdown(vkapi_driver_t* driver);

/**
 Get the supported Vk depth format for this device.
 @param driver A pointer to the driver instance.
 @return The supported depth format.
 */
VkFormat vkapi_driver_get_supported_depth_format(vkapi_driver_t* driver);

texture_handle_t vkapi_driver_create_tex2d(
    vkapi_driver_t* driver,
    VkFormat format,
    uint32_t width,
    uint32_t height,
    uint8_t mip_levels,
    uint8_t face_count,
    uint8_t array_count,
    VkImageUsageFlags usage_flags);

void vkapi_driver_destroy_tex2d(vkapi_driver_t* driver, texture_handle_t handle);

vkapi_rt_handle_t vkapi_driver_create_rt(
    vkapi_driver_t* driver,
    bool multiView,
    math_vec4f clear_col,
    vkapi_attach_info_t colours[VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT],
    vkapi_attach_info_t depth,
    vkapi_attach_info_t stencil);

vkapi_context_t* vakpi_driver_get_context(vkapi_driver_t* driver);

#endif