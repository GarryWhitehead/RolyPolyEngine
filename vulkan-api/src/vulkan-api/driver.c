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

#include "driver.h"

#include "error_codes.h"

#include <assert.h>
#include <string.h>
#include <utility/maths.h>

typedef struct VkApiDriver
{
    /// Current device context (instance, physical device, device).
    vkapi_context_t context;
    /// VMA instance.
    VmaAllocator vma_allocator;
    /// Semaphore used to signal that the image is ready for presentation.
    VkSemaphore image_ready_signal;

    vkapi_staging_pool_t staging_pool;

    vkapi_commands_t* commands;

    /** Private **/
    /// Permanent arena space for the lifetime of this driver.
    arena_t _perm_arena;
    /// Small scratch arena for limited lifetime allocations. Should be passed as a copy to
    /// functions for scoping only for the lifetime of that function.
    arena_t _scratch_arena;

    vkapi_res_cache_t* res_cache;
    arena_dyn_array_t render_targets;

} vkapi_driver_t;

 vkapi_driver_t* vkapi_driver_init(const char** instance_ext, uint32_t ext_count, int* error_code)
{ 
     *error_code = VKAPI_SUCCESS;

     // Allocate the arena space.
    arena_t perm_arena, scratch_arena;
    int perm_err = arena_new(VKAPI_PERM_ARENA_SIZE, &perm_arena);
    int scratch_err = arena_new(VKAPI_SCRATCH_ARENA_SIZE, &scratch_arena);
    if (perm_err != ARENA_SUCCESS || scratch_err != ARENA_SUCCESS)
    {
        *error_code = VKAPI_ERROR_INVALID_ARENA;
        return NULL;
    }

    vkapi_driver_t* driver = ARENA_MAKE_STRUCT(&perm_arena, vkapi_driver_t, ARENA_ZERO_MEMORY);
    assert(driver);
    driver->_perm_arena = perm_arena;
    driver->_scratch_arena = scratch_arena;

    VK_CHECK_RESULT(volkInitialize());

    vkapi_context_init(&driver->_perm_arena, &driver->context);
    driver->res_cache = vkapi_res_cache_init(&driver->_perm_arena);
    MAKE_DYN_ARRAY(vkapi_render_target_t , &driver->_perm_arena, 100, &driver->render_targets);

    // Create a new vulkan instance.
     *error_code = vkapi_context_create_instance(
        &driver->context, instance_ext, ext_count, &driver->_perm_arena, &driver->_scratch_arena);
     if (*error_code != VKAPI_SUCCESS)
     {
        
         return NULL;
     }
     volkLoadInstance(driver->context.instance);
     return driver;
}

int vkapi_driver_create_device(vkapi_driver_t* driver, VkSurfaceKHR surface)
{
    assert(driver);
    // prepare the vulkan backend
    int ret = vkapi_context_prepare_device(&driver->context, surface, &driver->_scratch_arena);
    if (ret != VKAPI_SUCCESS)
    {
        return ret;
    }

    driver->commands = vkapi_commands_init(&driver->context, &driver->_perm_arena);

    // set up the memory allocator
    VmaVulkanFunctions vk_funcs = {0};
    vk_funcs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vk_funcs.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo create_info = {0};
    create_info.vulkanApiVersion = VK_API_VERSION_1_2;
    create_info.physicalDevice = driver->context.physical;
    create_info.device = driver->context.device;
    create_info.instance = driver->context.instance;
    create_info.pVulkanFunctions = &vk_funcs;
    VkResult result = vmaCreateAllocator(&create_info, &driver->vma_allocator);
    assert(result == VK_SUCCESS);

    // create a semaphore for signaling that an image is ready for presentation
    VkSemaphoreCreateInfo sp_create_info = {0};
    sp_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreateSemaphore(
        driver->context.device, &sp_create_info, VK_NULL_HANDLE, &driver->image_ready_signal))

    return VKAPI_SUCCESS;
}

void vkapi_driver_shutdown(vkapi_driver_t* driver)
{
    vkapi_commands_destroy(&driver->context, driver->commands);
    vkDestroySemaphore(driver->context.device, driver->image_ready_signal, VK_NULL_HANDLE);
    vmaDestroyAllocator(driver->vma_allocator);

    vkapi_context_shutdown(&driver->context);
    arena_release(&driver->_scratch_arena);
    arena_release(&driver->_perm_arena);
}

VkFormat vkapi_driver_get_supported_depth_format(vkapi_driver_t* driver)
{
    // in order of preference - TODO: allow user to define whether stencil
    // format is required or not
    VkFormat formats[] = {
        VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT};
    VkFormatFeatureFlags format_feature = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkFormat output = VK_FORMAT_UNDEFINED;
    for (int i = 0; i < 3; ++i)
    {
        VkFormatProperties* props;
        vkGetPhysicalDeviceFormatProperties(driver->context.physical, formats[i], props);
        if (format_feature == (props->optimalTilingFeatures & format_feature))
        {
            output = formats[i];
            break;
        }
    }
    return output;
}

texture_handle_t vkapi_driver_create_tex2d(
    vkapi_driver_t* driver,
    VkFormat format,
    uint32_t width,
    uint32_t height,
    uint8_t mip_levels,
    uint8_t face_count,
    uint8_t array_count,
    VkImageUsageFlags usage_flags)
{
    return vkapi_res_cache_create_tex2d(
        driver->res_cache,
        &driver->context,
        format,
        width,
        height,
        mip_levels,
        face_count,
        array_count,
        usage_flags);
}

void vkapi_driver_destroy_tex2d(vkapi_driver_t* driver, texture_handle_t handle)
{
    vkapi_res_cache_delete_tex2d(driver->res_cache, handle);
}

vkapi_rt_handle_t vkapi_driver_create_rt(
    vkapi_driver_t* driver,
    bool multiView,
    math_vec4f clear_col,
    vkapi_attach_info_t colours[VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT],
    vkapi_attach_info_t depth,
    vkapi_attach_info_t stencil)
{
    assert(driver);
    vkapi_render_target_t rt = {
        rt.depth = depth,
        rt.stencil = stencil,
        rt.clear_colour.r = clear_col.r,
        rt.clear_colour.g = clear_col.g,
        rt.clear_colour.b = clear_col.b,
        rt.clear_colour.a = clear_col.a,
        .samples = 1,
        rt.multiView = multiView };
    memcpy(rt.colours, colours, sizeof(vkapi_attach_info_t) * VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT);

    vkapi_rt_handle_t handle = { .id = driver->render_targets.size };
    DYN_ARRAY_APPEND(&driver->render_targets, &rt);
    return handle;
}

vkapi_context_t* vakpi_driver_get_context(vkapi_driver_t* driver)
{
    assert(driver);
    return &driver->context;
}