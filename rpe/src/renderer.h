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

#ifndef __RPE_RENDERER_H__
#define __RPE_RENDERER_H__

#include <vulkan-api/driver.h>
#include <vulkan-api/renderpass.h>

typedef struct Engine rpe_engine_t;
typedef struct RenderGraph render_graph_t;
typedef struct MappedTexture rpe_mapped_texture_t;
typedef struct Scene rpe_scene_t;

typedef struct RenderTarget
{
    struct Attachment
    {
        rpe_mapped_texture_t* texture;
        uint8_t mipLevel;
        uint8_t layer;
    } attachments[VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT];

    uint8_t samples;
    vkapi_rt_handle_t handle;
    math_vec4f clear_col;
    enum LoadClearFlags load_flags[VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT];
    enum StoreClearFlags store_flags[VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT];
    uint32_t width;
    uint32_t height;
} rpe_render_target_t;

typedef struct Renderer
{
    render_graph_t* rg;

    rpe_engine_t* engine;

    // render targets for the backbuffer
    vkapi_rt_handle_t rt_handles[3];

    // keep track of the depth texture - set by createBackBufferRT
    texture_handle_t depth_handle;
} rpe_renderer_t;

void rpe_renderer_render(
    rpe_renderer_t* rdr,
    vkapi_driver_t* driver,
    rpe_engine_t* engine,
    rpe_scene_t* scene,
    float dt,
    bool clearSwap);

#endif
