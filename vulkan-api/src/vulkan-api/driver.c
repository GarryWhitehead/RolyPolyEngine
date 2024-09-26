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

int vkapi_driver_init(const char** instance_ext, uint32_t ext_count, vkapi_driver_t* driver)
{
    //memset(driver, 0, sizeof(struct VkApiDriver));

    // Allocate the arena space.
    int perm_err = arena_new(VKAPI_PERM_ARENA_SIZE, &driver->_perm_arena);
    int scratch_err = arena_new(VKAPI_SCRATCH_ARENA_SIZE, &driver->_scratch_arena);
    if (perm_err != ARENA_SUCCESS || scratch_err != ARENA_SUCCESS)
    {
        return VKAPI_ERROR_INVALID_ARENA;
    }

    vkapi_context_init(&driver->_perm_arena, &driver->context);

    // Create a new vulkan instance.
    return vkapi_context_create_instance(
        &driver->context, instance_ext, ext_count, &driver->_perm_arena, &driver->_scratch_arena);
}

int vkapi_driver_create_device(vkapi_driver_t* driver, VkSurfaceKHR surface)
{
    // prepare the vulkan backend
    int ret = vkapi_context_prepare_device(&driver->context, surface, &driver->_scratch_arena);
    if (ret != VKAPI_SUCCESS)
    {
        return ret;
    }

    driver->commands = vkapi_commands_init(&driver->context, &driver->_perm_arena);

    // set up the memory allocator
    VmaAllocatorCreateInfo create_info = {};
    create_info.vulkanApiVersion = VK_API_VERSION_1_2;
    create_info.physicalDevice = driver->context.physical;
    create_info.device = driver->context.device;
    create_info.instance = driver->context.instance;
    VkResult result = vmaCreateAllocator(&create_info, &driver->vma_allocator);
    assert(result == VK_SUCCESS);

    // create a semaphore for signaling that an image is ready for presentation
    VkSemaphoreCreateInfo sp_create_info = {};
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
        VkFormatProperties* props = VK_NULL_HANDLE;
        vkGetPhysicalDeviceFormatProperties(driver->context.physical, formats[i], props);
        if (format_feature == (props->optimalTilingFeatures & format_feature))
        {
            output = formats[i];
            break;
        }
    }
    return output;
}
