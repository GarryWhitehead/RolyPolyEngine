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

#include <stdbool.h>

typedef struct Renderer rpe_renderer_t;
typedef struct VkApiDriver vkapi_driver_t;
typedef struct Scene rpe_scene_t;
typedef struct RenderTarget rpe_render_target_t;
typedef struct Engine rpe_engine_t;

void rpe_renderer_begin_frame(rpe_renderer_t* r);

void rpe_renderer_end_frame(rpe_renderer_t* r);

void rpe_renderer_render_single_quad(
    rpe_renderer_t* rdr,
    rpe_render_target_t* rt,
    // FIXME: Need to hide this from public (using a material?).
    shader_prog_bundle_t* bundle,
    uint32_t multi_view_index);

void rpe_renderer_render_single_indexed(
    rpe_renderer_t* rdr,
    rpe_render_target_t* rt,
    // FIXME: Need to hide this from public (using a material?).
    shader_prog_bundle_t* bundle,
    buffer_handle_t vertex_buffer,
    buffer_handle_t index_buffer,
    uint32_t index_count,
    struct PushBlockEntry* pb_entries,
    uint32_t push_block_count,
    uint32_t multi_view_index);

void rpe_renderer_begin_renderpass(rpe_renderer_t* rdr, rpe_render_target_t* rt, uint32_t multi_view_count);

void rpe_render_end_renderpass(rpe_renderer_t* rdr);

void rpe_renderer_render(rpe_renderer_t* rdr, rpe_scene_t* scene, bool clear_swap);

#endif