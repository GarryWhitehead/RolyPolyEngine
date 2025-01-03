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

#ifndef __RPE_COLOUR_PASS_H__
#define __RPE_COLOUR_PASS_H__

#include "render_graph/render_graph_handle.h"

#include <vulkan-api/common.h>

// Forward declarations.
typedef struct RenderGraph render_graph_t;
typedef struct RenderGraphPass rg_pass_t;

struct DataGBuffer
{
    rg_handle_t pos;
    rg_handle_t normal;
    rg_handle_t emissive;
    rg_handle_t pbr;
    rg_handle_t depth;
    rg_handle_t colour;
    rg_handle_t rt;
};

struct GBufferLocalData
{
    uint32_t width;
    uint32_t height;
    VkFormat depth_format;
};

rg_handle_t
rpe_colour_pass_render(render_graph_t* rg, uint32_t width, uint32_t height, VkFormat depth_format);

#endif