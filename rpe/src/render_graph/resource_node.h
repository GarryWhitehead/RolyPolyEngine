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

#ifndef __RPE_RG_RESOURCE_NODE_H__
#define __RPE_RG_RESOURCE_NODE_H__

#include "render_graph_handle.h"
#include "render_graph_pass.h"
#include "dependency_graph.h"

#include <utility/arena.h>

// forward declerations
typedef struct PassNode rg_pass_node_t;
typedef struct Resource rg_resource_t;
typedef struct DependencyGraph rg_dep_graph_t;
typedef struct RenderGraphPass rg_pass_t;
typedef struct RenderGraph render_graph_t;
typedef struct VkApiDriver vkapi_driver_t;

typedef struct ResourceEdge
{
    /// Base edge pointer. This **must** be first in the struct fields.
    rg_edge_t base;
    VkImageUsageFlags usage;

} rg_resource_edge_t;

typedef struct ResourceNode
{
    /// Base node pointer. This **must** be first in the struct fields.
    rg_node_t base;
    /// The resource held by this node.
    rg_handle_t resource;
    /// The parent resource of this node if any.
    rg_handle_t parent;

    /// The pass which writes to this resource. Only one writer allowed.
    rg_resource_edge_t* writer_pass;
    /// The parent read edge of this node if any.
    rg_edge_t* parent_read_edge;
    /// The parent write edge of this node if any.
    rg_edge_t* parent_write_edge;

    rg_edge_t* alias_edge;

    /// Passes which read from this resource.
    arena_dyn_array_t reader_passes;

    // Set when running @sa compile().
    arena_dyn_array_t resources_to_bake;
    arena_dyn_array_t resources_to_destroy;

} rg_resource_node_t;

rg_resource_edge_t* rg_resource_edge_init(
    rg_dep_graph_t* dg, rg_node_t* from, rg_node_t* to, VkImageUsageFlags usage, arena_t* arena);

rg_resource_node_t*
rg_res_node_init(rg_dep_graph_t* dg, const char* name, arena_t* arena, rg_handle_t* parent);

rg_resource_edge_t* rg_res_node_get_writer_edge(rg_resource_node_t* rn, rg_pass_node_t* node);

void rg_res_node_set_writer_edge(rg_resource_node_t* rn, rg_resource_edge_t* edge);

rg_resource_edge_t* rg_res_node_get_reader_edge(rg_resource_node_t* rn, rg_pass_node_t* node);

void rg_res_node_set_reader_edge(rg_resource_node_t* rn, rg_resource_edge_t* edge);

void rg_res_node_add_resource_to_bake(rg_resource_node_t* rn, rg_resource_t* r);

void rg_res_node_add_resource_to_destroy(rg_resource_node_t* rn, rg_resource_t* r);

void rg_res_node_bake_resources(rg_resource_node_t* rn, vkapi_driver_t* driver);

void rg_res_node_destroy_resources(rg_resource_node_t* rn, vkapi_driver_t* driver);

bool rg_res_node_has_writer_pass(rg_resource_node_t* rn);

bool rg_res_node_has_readers(rg_resource_node_t* rn);

bool rg_res_node_has_writers(rg_resource_node_t* rn, rg_dep_graph_t* dg, arena_t* arena);

rg_resource_node_t* rg_res_node_get_parent_node(rg_resource_node_t* rn, render_graph_t* rg);

bool rg_res_node_set_parent_reader(
    rg_dep_graph_t* dg, rg_resource_node_t* rn, rg_resource_node_t* parent, arena_t* arena);

bool rg_res_node_set_parent_writer(
    rg_dep_graph_t* dg, rg_resource_node_t* rn, rg_resource_node_t* parent, arena_t* arena);

void rg_res_node_update_res_usage(rg_resource_node_t* rn, render_graph_t* rg, rg_dep_graph_t* dg);

void rg_res_node_set_alias_res_edge(
    rg_dep_graph_t* dg, rg_resource_node_t* rn, rg_resource_node_t* alias, arena_t* arena);

#endif
