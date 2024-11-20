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

#ifndef __VKAPI_COMMON_H__
#define __VKAPI_COMMON_H__

#include <assert.h>
#include <log.h>
#include <vma/vma_common.h>

#ifndef NDEBUG
#ifdef RPE_RENDERDOC_LIB_PATH
#include <renderdoc_app.h>
extern RENDERDOC_API_1_1_0 *rdoc_api;
#if __linux__
#include <dlfcn.h>
#elif WIN32
#include <Windows.h>
#endif
#endif
#endif

// threading info (not used at present)
#define VULKAN_THREADED 1
#define VULKAN_THREADED_GROUP_SIZE 512

#define VK_CHECK_RESULT(f)                                                                         \
    {                                                                                              \
        VkResult res = (f);                                                                        \
        if (res != VK_SUCCESS)                                                                     \
        {                                                                                          \
            log_error(                                                                             \
                "Fatal : VkResult returned error code %i at %s at line %i.\n",                     \
                (int)res,                                                                          \
                __FILE__,                                                                          \
                __LINE__);                                                                         \
            assert(res == VK_SUCCESS);                                                             \
        }                                                                                          \
    }

// VMA doesn't use the c++ bindings, so this is specifically for dealing with
// VMA functions
#define VMA_CHECK_RESULT(f)                                                                        \
    {                                                                                              \
        VkResult res = (f);                                                                        \
        if (res != VK_SUCCESS)                                                                     \
        {                                                                                          \
            printf(                                                                                \
                "Fatal : VkResult returned error code at %s at line %i.\n", __FILE__, __LINE__);   \
            assert(res == VK_SUCCESS);                                                             \
        }                                                                                          \
    }

// RenderDoc API macros. Only defined in debug builds.
#ifndef NDEBUG
//#ifdef RPE_RENDERDOC_LIB_PATH
#ifdef __linux__
static inline void create_renderdoc_instance()
{
    void* mod = dlopen(RPE_RENDERDOC_LIB_PATH, RTLD_NOW);
    if (mod)
    {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)dlsym(mod, "RENDERDOC_GetAPI");
        int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_0, (void**)&rdoc_api);
        assert(ret == 1);
    }
}
#elif WIN32
static inline void create_renderdoc_instance()
{
    HMODULE mod = GetModuleHandleA("renderdoc.dll");
    if (mod)
    {
        log_info("RenderDoc debugging enabled.");
        pRENDERDOC_GetAPI RENDERDOC_GetAPI =
            (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
        int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_0, (void**)&rdoc_api);
        assert(ret == 1);
    }
}
#endif
static inline void renderdoc_start_capture(RENDERDOC_DevicePointer device, RENDERDOC_WindowHandle wnd_handle)
{
    if (rdoc_api)
    {
        rdoc_api->StartFrameCapture(device, wnd_handle);
    }
}

static inline void renderdoc_stop_capture(RENDERDOC_DevicePointer device, RENDERDOC_WindowHandle wnd_handle)
{
    if (rdoc_api)
    {
        rdoc_api->EndFrameCapture(device, wnd_handle);
    }
}

#define RENDERDOC_CREATE_API_INSTANCE create_renderdoc_instance();
#define RENDERDOC_START_CAPTURE(device, win_handle) renderdoc_start_capture(device, win_handle);
#define RENDERDOC_END_CAPTURE(device, win_handle) renderdoc_stop_capture(device, win_handle);
#endif
//#endif
//#else
//#define RENDERDOC_CREATE_API_INSTANCE
//#define RENDERDOC_START_CAPTURE(device, win_handle)
//#define RENDERDOC_END_CAPTURE(device, win_handle)
#endif