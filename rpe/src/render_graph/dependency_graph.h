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

#ifndef __RPE_RG_DEPENDENCY_GRAPH_H__
#define __RPE_RG_DEPENDENCY_GRAPH_H__

#include <stddef.h>
#include <utility/arena.h>
#include <utility/string.h>

// forward declarations
typedef struct DependencyGraph rg_dep_graph_t;

typedef struct Edge
{
    // the node id that this edge projects from
    size_t from_id;
    // the node id that this edge projects to
    size_t to_id;
} rg_edge_t;

typedef struct Node
{
    int ref_count;
    string_t name;
    uint64_t id;
} rg_node_t;

rg_node_t* rg_node_init(rg_dep_graph_t* dg, const char* name, arena_t* arena);

void rg_node_declare_side_effect(rg_node_t* node);

bool rg_node_is_culled(rg_node_t* node);

rg_edge_t* rg_edge_init(rg_dep_graph_t* dg, rg_node_t* from, rg_node_t* to, arena_t* arena);

rg_dep_graph_t* rg_dep_graph_init(arena_t* arena);

arena_dyn_array_t
rg_dep_graph_get_writer_edges(rg_dep_graph_t* dg, rg_node_t* node, arena_t* arena);

arena_dyn_array_t
rg_dep_graph_get_reader_edges(rg_dep_graph_t* dg, rg_node_t* node, arena_t* arena);

bool rg_dep_graph_is_valid_edge(rg_dep_graph_t* dg, rg_edge_t* edge);

rg_node_t* rg_dep_graph_get_node(rg_dep_graph_t* dg, size_t id);

size_t rg_dep_graph_create_id(rg_dep_graph_t* dg);

void rg_dep_graph_add_node(rg_dep_graph_t* dg, rg_node_t* node);

void rg_dep_graph_add_edge(rg_dep_graph_t* dg, rg_edge_t* edge);

void rg_dep_graph_cull(rg_dep_graph_t* dg, arena_t* scratch_arena);

void rg_dep_graph_clear(rg_dep_graph_t* dg);

void rg_dep_graph_export_graph_viz(rg_dep_graph_t* dg, string_t output, arena_t* arena);

string_t rg_node_get_graph_viz(rg_node_t* n, arena_t* arena);

#endif
