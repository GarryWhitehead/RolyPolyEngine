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

#include "rendergraph_resource.h"

#include "render_graph.h"
#include "render_pass_node.h"
#include "resource_node.h"
#include "resources.h"
#include <vulkan-api/resource_cache.h>

#include <assert.h>

rg_resource_t* rg_res_get_resource(rg_render_graph_resource_t* r, rg_handle_t handle)
{
    assert(rg_handle_is_valid(handle));
    return rg_get_resource(r->rg, handle);
}

rg_resource_info_t rg_res_get_render_pass_info(rg_render_graph_resource_t* r, rg_handle_t handle)
{
    assert(rg_handle_is_valid(handle));
    rg_pass_info_t info = rg_render_pass_node_get_rt_info(r->pass_node, handle);
    rg_resource_info_t out = { .data = info.vkapi_rpass_data, .handle = info.desc.rt_handle };
    return out;
}

texture_handle_t rg_res_get_tex_handle(rg_render_graph_resource_t* r, rg_handle_t handle)
{
    assert(rg_handle_is_valid(handle));
    rg_texture_resource_t* t_res = (rg_texture_resource_t*)rg_get_resource(r->rg, handle);
    assert(vkapi_tex_handle_is_valid(t_res->handle));
    return t_res->handle;
}

