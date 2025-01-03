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

#ifndef __VKAPI_CONTEXT_H__
#define __VKAPI_CONTEXT_H__

#include "common.h"

#include <stdbool.h>
#include <utility/arena.h>
#include <utility/compiler.h>

#define VKAPI_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"

/**
 The current state of this vulkan instance. Encapsulates all
 information extracted from the device and physical device.
 *
 */
typedef struct VkApiContext
{
    /**
     Queue family indices.
     */
    struct QueueInfo
    {
        uint32_t compute;
        uint32_t present;
        uint32_t graphics;
    } queue_info;

    /**
     Extensions which are available on this device (and we are interested in).
     */
    struct Extensions
    {
        bool has_physical_dev_props2;
        bool has_external_capabilities;
        bool has_debug_utils;
        bool has_multi_view;
    } extensions;

    VkInstance instance;
    VkDevice device;
    VkPhysicalDevice physical;

    VkQueue graphics_queue;
    VkQueue present_queue;
    VkQueue compute_queue;

    // An array of validation layer names, passed when creating the device.
    arena_dyn_array_t req_layers;

#ifdef VULKAN_VALIDATION_DEBUG

    VkDebugReportCallbackEXT debug_callback;
    VkDebugUtilsMessengerEXT debug_messenger;
#endif

} vkapi_context_t;

/**
 Initialise a new Vulkan context.
 @param perm_arena A permanent arena lifetime allocator.
 @param new_context A pointer to the context struct to initialise.
 */
vkapi_context_t* vkapi_context_init(arena_t* perm_arena);

/**
 Create a Vulkan instance. This must be called before creating the Vulkan device.
 @param context A pointer to an initialised context struct.
 @param glfw_ext A list of GLFW extension names.
 @param glfw_ext_count The number of GLFW extensions.
 @param arena A pointer to a permanent lifetime arena.
 @param scratch_arena A pointer to a scratch arena - only required for the scope of this function.
 @returns a VKAPI error code.
 */
int vkapi_context_create_instance(
    vkapi_context_t* context,
    const char** glfw_ext,
    uint32_t glfw_ext_count,
    arena_t* arena,
    arena_t* scratch_arena);

/**

 @param context
 @param flags
 @param reqs
 @return
 */
uint32_t
vkapi_context_select_mem_type(vkapi_context_t* context, uint32_t flags, VkMemoryPropertyFlags reqs);

/**
 Destroy the resource used by a Vulkan context.
 @param context A pointer to an initialised context struct.
 */
void vkapi_context_shutdown(vkapi_context_t* context, VkSurfaceKHR surface);

/** Private functions **/

int vkapi_context_prepare_device(
    vkapi_context_t* context, VkSurfaceKHR win_surface, arena_t* scratch_arena);

#endif
