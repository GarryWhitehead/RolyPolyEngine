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

#include "swapchain.h"

#include "context.h"
#include "driver.h"
#include "error_codes.h"
#include "texture.h"

#include <math.h>
#include <string.h>
#include <utility/arena.h>

vkapi_swapchain_t vkapi_swapchain_init()
{
    vkapi_swapchain_t swapchain;
    memset(&swapchain, 0, sizeof(vkapi_swapchain_t));
    return swapchain;
}

void vkapi_swapchain_destroy(vkapi_driver_t* driver, vkapi_swapchain_t* swapchain)
{
    // The swapchain images are held by the resource cache and cleared up over there.
    vkDestroySwapchainKHR(driver->context->device, swapchain->sc_instance, VK_NULL_HANDLE);
}

int vkapi_swapchain_create(
    vkapi_driver_t* driver,
    vkapi_swapchain_t* swapchain,
    VkSurfaceKHR surface,
    uint32_t win_width,
    uint32_t win_height,
    arena_t* scratch_arena)
{
    VkDevice device = driver->context->device;
    VkPhysicalDevice gpu = driver->context->physical;

    // Get the basic surface properties of the physical device
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &capabilities);

    uint32_t surface_format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &surface_format_count, VK_NULL_HANDLE);
    VkSurfaceFormatKHR* surface_formats =
        ARENA_MAKE_ARRAY(scratch_arena, VkSurfaceFormatKHR, surface_format_count, 0);
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &surface_format_count, surface_formats);

    uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &present_mode_count, VK_NULL_HANDLE);
    VkPresentModeKHR* present_modes =
        ARENA_MAKE_ARRAY(scratch_arena, VkPresentModeKHR, present_mode_count, 0);
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &present_mode_count, present_modes);

    // make sure that we have suitable swap chain extensions available before
    // continuing
    if (!surface_format_count || !present_mode_count)
    {
        return VKAPI_ERROR_NO_SWAPCHAIN;
    }

    // Next step is to determine the surface format. Ideally a undefined format is
    //  preferred, so we can set our own, otherwise we will go with one that suits
    // our colour needs - i.e. 8-bit BGRA and SRGB.
    VkSurfaceFormatKHR req_surface_formats;

    if (surface_formats[0].format == VK_FORMAT_UNDEFINED)
    {
        req_surface_formats.format = VK_FORMAT_B8G8R8A8_UNORM;
        req_surface_formats.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    }
    else
    {
        for (uint32_t i = 0; i < surface_format_count; ++i)
        {
            if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM &&
                surface_formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
            {
                req_surface_formats = surface_formats[i];
                break;
            }
        }
    }
    swapchain->surface_format = req_surface_formats;

    // And then the presentation format - the preferred format is triple
    // buffering; immediate mode is only supported on some drivers.
    VkPresentModeKHR req_present_mode;
    for (uint32_t i = 0; i < present_mode_count; ++i)
    {
        if (present_modes[i] == VK_PRESENT_MODE_FIFO_KHR ||
            present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            req_present_mode = present_modes[i];
            break;
        }
    }

    // Set the resolution of the swap chain buffers...
    // First of check if we can manually set the dimension - some GPUs allow
    // this by setting the max as the size of uint32.
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        swapchain->extent = capabilities.currentExtent;
    }
    else
    {
        swapchain->extent.width = fmax(
            capabilities.minImageExtent.width, fmin(capabilities.maxImageExtent.width, win_width));
        swapchain->extent.height = fmax(
            capabilities.minImageExtent.height,
            fmin(capabilities.maxImageExtent.height, win_height));
    }

    // Get the number of possible images we can send to the queue.
    uint32_t image_count =
        capabilities.minImageCount + 1; // adding one as we would like to implement triple buffering
    if (capabilities.maxImageCount > 0 && image_count > capabilities.maxImageCount)
    {
        image_count = capabilities.maxImageCount;
    }

    int composite_flag = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if (capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
    {
        composite_flag = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    }

    // if the graphics and presentation aren't the same, then use concurrent
    // sharing mode.
    VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
    if (driver->context->queue_info.graphics != driver->context->queue_info.present)
    {
        sharing_mode = VK_SHARING_MODE_CONCURRENT;
    }

    VkSwapchainCreateInfoKHR createInfo = {0};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = image_count;
    createInfo.imageFormat = req_surface_formats.format;
    createInfo.imageColorSpace = req_surface_formats.colorSpace;
    createInfo.imageExtent = swapchain->extent, createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = sharing_mode;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = composite_flag;
    createInfo.presentMode = req_present_mode;
    createInfo.clipped = VK_TRUE;

    // And finally, create the swap chain.
    VK_CHECK_RESULT(
        vkCreateSwapchainKHR(device, &createInfo, VK_NULL_HANDLE, &swapchain->sc_instance));

    vkapi_swapchain_prepare_views(driver, swapchain, scratch_arena);

    arena_reset(scratch_arena);
    return VKAPI_SUCCESS;
}

void vkapi_swapchain_prepare_views(
    vkapi_driver_t* driver, vkapi_swapchain_t* swapchain, arena_t* scratch_arena)
{
    // Get the image locations created when creating the swap chain.
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(
        driver->context->device, swapchain->sc_instance, &swapchain->image_count, VK_NULL_HANDLE));
    VkImage* images = ARENA_MAKE_ARRAY(scratch_arena, VkImage, swapchain->image_count, 0);
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(
        driver->context->device, swapchain->sc_instance, &swapchain->image_count, images));

    assert(swapchain->image_count <= VKAPI_SWAPCHAIN_MAX_IMAGE_COUNT);
    for (uint32_t i = 0; i < swapchain->image_count; ++i)
    {
        swapchain->contexts[i].handle = vkapi_res_push_reserved_tex2d(
            driver->res_cache,
            driver->context,
            swapchain->extent.width,
            swapchain->extent.height,
            swapchain->surface_format.format,
            i,
            0,
            &images[i]);
    }
}
