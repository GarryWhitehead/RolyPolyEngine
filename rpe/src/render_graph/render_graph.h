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

#ifndef __RPE_RG_RENDER_GRAPH_H__
#define __RPE_RG_RENDER_GRAPH_H__

#include "backboard.h"
#include "render_graph_handle.h"
#include "render_graph_pass.h"
#include "resources.h"

#include <utility/arena.h>
#include <vulkan-api/common.h>
#include <vulkan-api/renderpass.h>

// Forward declarations.
typedef struct RenderGraph render_graph_t;
typedef struct Resource rg_resource_t;
typedef struct ResourceNode rg_resource_node_t;
typedef struct DependencyGraph rg_dep_graph_t;
typedef struct RenderPassNode rg_render_pass_node_t;
typedef struct RenderGraphPass rg_pass_t;

typedef void(setup_func)(render_graph_t*, rg_pass_node_t*, void*, void*);

typedef struct ResourceSlot
{
    size_t resource_idx;
    size_t node_idx;
} rg_resource_slot_t;

typedef struct RenderGraph
{
    rg_dep_graph_t* dep_graph;

    rg_backboard_t backboard;

    /// a list of all the render passes
    arena_dyn_array_t rg_passes;

    /// a virtual list of all the resources associated with this graph
    arena_dyn_array_t resources;

    arena_dyn_array_t pass_nodes;
    arena_dyn_array_t resource_nodes;

    arena_dyn_array_t resource_slots;

    /// Arena for memory allocations (frame scope).
    arena_t* arena;
    /// Number of active (non-culled) pass nodes set after a call to @sa rg_compile.
    size_t active_idx;

} render_graph_t;

render_graph_t* rg_init(arena_t* arena);

rg_render_pass_node_t*
rg_create_pass_node(render_graph_t* rg, const char* name, rg_pass_t* rg_pass);

void rg_add_present_pass(render_graph_t* rg, rg_handle_t handle);

rg_handle_t rg_add_resource(render_graph_t* rg, rg_resource_t* r, rg_handle_t* parent);

rg_handle_t rg_move_resource(render_graph_t* rg, rg_handle_t from, rg_handle_t to);

rg_resource_t* rg_get_resource(render_graph_t* rg, rg_handle_t handle);

rg_resource_node_t* rg_get_resource_node(render_graph_t* rg, rg_handle_t handle);

rg_handle_t rg_import_render_target(
    render_graph_t* rg, const char* name, rg_import_rt_desc_t desc, vkapi_rt_handle_t handle);

rg_handle_t rg_add_read(
    render_graph_t* rg, rg_handle_t handle, rg_pass_node_t* pass_node, VkImageUsageFlags usage);

rg_handle_t rg_add_write(
    render_graph_t* rg, rg_handle_t handle, rg_pass_node_t* pass_node, VkImageUsageFlags usage);

render_graph_t* rg_compile(render_graph_t* rg);

void rg_execute(render_graph_t* rg, vkapi_driver_t* driver, rpe_engine_t* engine);

rg_pass_t* rg_add_pass(
    render_graph_t* rg,
    const char* name,
    setup_func setup,
    execute_func execute,
    size_t data_size,
    void* local_data);

void rg_add_executor_pass(render_graph_t* rg, const char* name, execute_func execute);

arena_t* rg_get_arena(render_graph_t* rg);

rg_dep_graph_t* rg_get_dep_graph(render_graph_t* rg);

rg_backboard_t* rg_get_backboard(render_graph_t* rg);

void rg_clear(render_graph_t* rg);

#endif
