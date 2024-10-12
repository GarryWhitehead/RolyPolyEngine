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

#ifndef __RPE_RG_RESOURCE_H__
#define __RPE_RG_RESOURCE_H__

#include "render_graph_handle.h"
#include "vulkan-api/renderpass.h"

// forward declarations
typedef struct RenderPassNode rg_render_pass_node_t;
typedef struct RenderGraph rg_render_graph_t;
typedef struct Resource rg_resource_t;

typedef struct RenderGraphResourceInfo
{
    vkapi_render_pass_data_t data;
    vkapi_rt_handle_t handle;
} rg_resource_info_t;

typedef struct RenderGraphResource
{
   rg_render_graph_t* rg;
    rg_render_pass_node_t* pass_node;
} rg_render_graph_resource_t;

rg_resource_t* rg_res_get_resource(rg_render_graph_resource_t* r, rg_handle_t handle);

rg_resource_info_t rg_res_get_render_pass_info(rg_render_graph_resource_t* r, rg_handle_t handle);

texture_handle_t rg_res_get_tex_handle(rg_render_graph_resource_t* r, rg_handle_t handle);

#endif
