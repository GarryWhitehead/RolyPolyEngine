/* Copyright (c) 2022 Garry Whitehead
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

#include "dependency_graph.h"

#include "render_graph/render_pass_node.h"

typedef struct DependencyGraph
{
    // The nodes are not owned by the dependency graph so we only
    // keep reference with a raw pointer. Thus, its imperative that
    // the owner only destroys the node after finishing with the
    // dependency graph.
    arena_dyn_array_t nodes;

    // As nodes, edges are not ownded by the dependency graph but the
    // render graph, so be careful with the lifetime of the edge.
    arena_dyn_array_t edges;
} rg_dep_graph_t;

rg_node_t* rg_node_init(rg_dep_graph_t* dg, const char* name, arena_t* arena)
{
    assert(dg);
    rg_node_t* i = ARENA_MAKE_STRUCT(arena, rg_node_t, ARENA_ZERO_MEMORY);
    i->name = string_init(name, arena);
    i->id = rg_dep_graph_create_id(dg);
    rg_dep_graph_add_node(dg, i);
    return i;
}

void rg_node_declare_side_effect(rg_node_t* node)
{
    assert(node);
    node->ref_count = 0x7FFF;
}

bool rg_node_is_culled(rg_node_t* node)
{
    assert(node);
    return node->ref_count == 0;
}

rg_edge_t* rg_edge_init(rg_dep_graph_t* dg, rg_node_t* from, rg_node_t* to, arena_t* arena)
{
    assert(dg);
    assert(from);
    assert(to);
    rg_edge_t* i = ARENA_MAKE_STRUCT(arena, rg_edge_t, ARENA_ZERO_MEMORY);
    i->from_id = from->id;
    i->to_id = to->id;
    rg_dep_graph_add_edge(dg, i);
    return i;
}

rg_dep_graph_t* rg_dep_graph_init(arena_t* arena)
{
    rg_dep_graph_t* dg = ARENA_MAKE_STRUCT(arena, rg_dep_graph_t, ARENA_ZERO_MEMORY);
    MAKE_DYN_ARRAY(rg_node_t*, arena, 30, &dg->nodes);
    MAKE_DYN_ARRAY(rg_edge_t*, arena, 30, &dg->edges);
    return dg;
}

void rg_dep_graph_add_node(rg_dep_graph_t* dg, rg_node_t* node)
{
    assert(dg);
    assert(node);
    DYN_ARRAY_APPEND(&dg->nodes, &node);
}

size_t rg_dep_graph_create_id(rg_dep_graph_t* dg)
{
    assert(dg);
    return dg->nodes.size;
}

rg_node_t* rg_dep_graph_get_node(rg_dep_graph_t* dg, size_t id)
{
    assert(dg);
    assert(id < dg->nodes.size);
    return DYN_ARRAY_GET(rg_node_t*, &dg->nodes, id);
}

void rg_dep_graph_add_edge(rg_dep_graph_t* dg, rg_edge_t* edge)
{
    assert(dg);
    DYN_ARRAY_APPEND(&dg->edges, &edge);
}

bool rg_dep_graph_is_valid_edge(rg_dep_graph_t* dg, rg_edge_t* edge)
{
    rg_node_t* from = DYN_ARRAY_GET(rg_node_t*, &dg->nodes, edge->from_id);
    rg_node_t* to = DYN_ARRAY_GET(rg_node_t*, &dg->nodes, edge->to_id);
    return !rg_node_is_culled(from) && !rg_node_is_culled(to);
}

arena_dyn_array_t rg_dep_graph_get_reader_edges(rg_dep_graph_t* dg, rg_node_t* node, arena_t* arena)
{
    assert(dg);
    assert(node);

    arena_dyn_array_t out;
    MAKE_DYN_ARRAY(rg_edge_t*, arena, 30, &out);
    for (size_t i = 0; i < dg->edges.size; ++i)
    {
        rg_edge_t* edge = DYN_ARRAY_GET(rg_edge_t*, &dg->edges, i);
        if (edge->to_id == node->id)
        {
            DYN_ARRAY_APPEND(&out, &edge);
        }
    }
    return out;
}

arena_dyn_array_t rg_dep_graph_get_writer_edges(rg_dep_graph_t* dg, rg_node_t* node, arena_t* arena)
{
    assert(dg);
    assert(node);

    arena_dyn_array_t out;
    MAKE_DYN_ARRAY(rg_edge_t*, arena, 30, &out);
    for (size_t i = 0; i < dg->edges.size; ++i)
    {
        rg_edge_t* edge = DYN_ARRAY_GET(rg_edge_t*, &dg->edges, i);
        if (edge->from_id == node->id)
        {
            DYN_ARRAY_APPEND(&out, &edge);
        }
    }
    return out;
}

void rg_dep_graph_cull(rg_dep_graph_t* dg, arena_t* scratch_arena)
{
    assert(dg);

    // increase the reference count for all nodes that have a writer
    for (size_t i = 0; i < dg->edges.size; ++i)
    {
        rg_edge_t* edge = DYN_ARRAY_GET(rg_edge_t*, &dg->edges, i);
        assert(edge->from_id < dg->nodes.size);
        rg_node_t* node = DYN_ARRAY_GET(rg_node_t*, &dg->nodes, edge->from_id);
        node->ref_count++;
    }

    arena_dyn_array_t nodes_to_cull;
    MAKE_DYN_ARRAY(rg_node_t*, scratch_arena, 30, &nodes_to_cull);
    for (size_t i = 0; i < dg->nodes.size; ++i)
    {
        rg_node_t* node = DYN_ARRAY_GET(rg_node_t*, &dg->nodes, i);
        if (!node->ref_count)
        {
            DYN_ARRAY_APPEND(&nodes_to_cull, &node);
        }
    }

    while (nodes_to_cull.size > 0)
    {
        rg_node_t* node = DYN_ARRAY_POP_BACK(rg_node_t*, &nodes_to_cull);
        arena_dyn_array_t reader_edges = rg_dep_graph_get_reader_edges(dg, node, scratch_arena);
        for (size_t i = 0; i < reader_edges.size; ++i)
        {
            rg_edge_t* edge = DYN_ARRAY_GET(rg_edge_t*, &reader_edges, i);
            // remove any linked nodes that have no reference after the
            // culling of the parent node.
            rg_node_t* child_node = rg_dep_graph_get_node(dg, edge->from_id);
            child_node->ref_count--;
            assert(child_node->ref_count >= 0);
            if (!child_node->ref_count)
            {
                DYN_ARRAY_APPEND(&nodes_to_cull, &child_node);
            }
        }
    }
}

void rg_dep_graph_clear(rg_dep_graph_t* dg)
{
    assert(dg);
    dyn_array_clear(&dg->nodes);
    dyn_array_clear(&dg->edges);
}

void rg_dep_graph_export_graph_viz(rg_dep_graph_t* dg, string_t output, arena_t* arena)
{
    assert(dg);
    output = string_append(&output, "digraph \"rendergraph\" { \n", arena);
    output = string_append(&output, "bgcolor = white\n", arena);
    output = string_append(&output, "node [shape=rectangle, fontname=\"arial\", fontsize=12]\n", arena);

    // add each node
    for (size_t i = 0; i < dg->nodes.size; ++i)
    {
        rg_node_t* n = DYN_ARRAY_GET_PTR(rg_node_t, &dg->nodes, i);
        string_t node_str = rg_node_get_graph_viz(n, arena);
        sprintf(output.data, "\"N %lu \" %s \n", n->id, node_str.data);
    }
    output = string_append(&output, "\n", arena);

    for (size_t i = 0; i < dg->nodes.size; ++i)
    {
        rg_node_t* n = DYN_ARRAY_GET_PTR(rg_node_t, &dg->nodes, i);
        arena_dyn_array_t writer_edges = rg_dep_graph_get_writer_edges(dg, n, arena);
        string_t valid_str = {.len = 0};
        string_t invalid_str = {.len = 0};

        for (size_t j = 0; j < writer_edges.size; ++j)
        {
            rg_edge_t* edge = DYN_ARRAY_GET_PTR(rg_edge_t, &writer_edges, j);
            rg_node_t* link = rg_dep_graph_get_node(dg, edge->to_id);
            if (rg_dep_graph_is_valid_edge(dg, edge))
            {
                if (!valid_str.len)
                {
                    sprintf(valid_str.data, "N%lu -> { ", n->id);
                }
                sprintf(valid_str.data, "N%lu ", link->id);
            }
            else
            {
                if (!invalid_str.len)
                {
                    sprintf(invalid_str.data, "N%lu -> { ", n->id);
                }
                sprintf(invalid_str.data, "N%lu ", link->id);
            }
        }
        if (!valid_str.len)
        {
            valid_str = string_append(&valid_str, "} [color=red4]\n", arena);
            output = string_append(&output, valid_str.data, arena);
        }

        if (!invalid_str.len)
        {
            invalid_str = string_append(&invalid_str, "} [color=red4 style=dashed]\n", arena);
            output = string_append(&output, invalid_str.data, arena);
        }
    }

    output = string_append(&output, "}\n", arena);
}

string_t rg_node_get_graph_viz(rg_node_t* n, arena_t* arena)
{
    assert(n);
    string_t out;
    out.data = ARENA_MAKE_ARRAY(arena, char, 1024, ARENA_ZERO_MEMORY);
    sprintf(out.data, "[label=\"node\\n name: %s id: %i, refCount: %i\",", n->name.data, n->id, n->ref_count);
    out = string_append(&out, " style=filled, fillcolor=green]", arena);
    return out;
}

