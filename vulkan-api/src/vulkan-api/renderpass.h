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

#ifndef __VKAPI_RENDERPASS_H__
#define __VKAPI_RENDERPASS_H__

#include "common.h"
#include "resource_cache.h"
#include "backend/enums.h"
#include <utility/maths.h>

#define VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT 6
// Allow for depth and stencil info
#define VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT 8

#define VKAPI_RENDER_TARGET_DEPTH_INDEX 7
#define VKAPI_RENDER_TARGET_STENCIL_INDEX 8

// Forward declarations.

typedef struct RenderTargetHandle
{
    uint32_t id;
} vkapi_rt_handle_t;

typedef struct AttachmentInfo
{
    uint8_t layer;
    uint8_t level;
    texture_handle_t handle;
} vkapi_attach_info_t;

typedef struct RenderTarget
{
    vkapi_attach_info_t depth;
    vkapi_attach_info_t stencil;
    vkapi_attach_info_t colours[VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT];
    math_vec4f clear_colour;
    uint8_t samples;
    bool multiView;

} vkapi_render_target_t;

/**
 Used for building a concrete vulkan renderpass. The data is obtained
 from the RenderGraph side.
 */
typedef struct RenderPassData
{
    enum LoadClearFlags load_clear_flags[VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT];
    enum StoreClearFlags store_clear_flags[VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT];
    // Initial layout is usually undefined, but needs to be the layout used in the previous pass
    // when load-clear flags are set to 'load'.
    VkImageLayout init_layouts[VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT];
    VkImageLayout final_layouts[VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT];
    uint32_t width;
    uint32_t height;
    math_vec4f clear_col;
} vkapi_render_pass_data_t;

#endif