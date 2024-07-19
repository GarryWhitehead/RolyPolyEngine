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

#ifndef __VKAPI_ERROR_CODES_H__
#define __VKAPI_ERROR_CODES_H__

enum
{
    VKAPI_SUCCESS,
    VKAPI_ERROR_NO_DEVICE,
    VKAPI_ERROR_NO_VK_INSTANCE,
    VKAPI_ERROR_MISSING_GFLW_EXT,
    VKAPI_ERROR_NO_SUPPORTED_GPU,
    VKAPI_ERROR_NO_GRAPHIC_QUEUE,
    VKAPI_ERROR_PRESENT_QUEUE_NOT_SUPPORTED,
    VKAPI_ERROR_SWAPCHAIN_NOT_FOUND,
    VKAPI_ERROR_INVALID_ARENA,
    VKAPI_ERROR_NO_SWAPCHAIN
};

static const char* get_error_str(int code)
{
    switch (code)
    {
        case VKAPI_ERROR_MISSING_GFLW_EXT:
            return "Unable to find required extension properties for GLFW.";
        case VKAPI_ERROR_NO_VK_INSTANCE:
            return "Vulkan instance has not been created.";
        case VKAPI_ERROR_NO_SUPPORTED_GPU:
            return "No Vulkan supported GPU devices were found.";
        case VKAPI_ERROR_NO_GRAPHIC_QUEUE:
            return "Unable to find a suitable graphics queue on the device.";
        case VKAPI_ERROR_PRESENT_QUEUE_NOT_SUPPORTED:
            return "Physical device does not support a presentation queue.";
        case VKAPI_ERROR_SWAPCHAIN_NOT_FOUND:
            return "Swap chain extension not found.";
        case VKAPI_ERROR_NO_SWAPCHAIN:
            return "Unable to locate suitable swap chains on device.";
        default:
            return "Unknown error code.";
    }
    return "";
}

#endif