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

#include "dependency_graph.h"

#include <stdio.h>

string_t rg_node_get_graph_viz(rg_node_t* n, arena_t* arena)
{
    assert(n);
    char line_buffer[1024];
    string_t out;
    out.data = ARENA_MAKE_ARRAY(arena, char, 1024, ARENA_ZERO_MEMORY);
    sprintf(
        line_buffer,
        "[label=\"node\\n name: %s id: %i, refCount: %i\", style=filled, fillcolor=green]",
        n->name.data,
        n->id,
        n->ref_count);
    return string_init(line_buffer, arena);
}

string_t rg_dep_graph_export_graph_viz(rg_dep_graph_t* dg, arena_t* arena)
{
    assert(dg);
    char line_buffer[1024];

    sprintf(
        line_buffer,
        "digraph \"rendergraph\" { \nbgcolor = white\nnode [shape=rectangle, fontname=\"arial\", "
        "fontsize=12]\n");
    string_t output = string_init((const char*)line_buffer, arena);

    // add each node
    for (size_t i = 0; i < dg->nodes.size; ++i)
    {
        rg_node_t* n = DYN_ARRAY_GET(rg_node_t*, &dg->nodes, i);
        string_t node_str = rg_node_get_graph_viz(n, arena);
        sprintf(line_buffer, "\"N %lu \" %s \n", n->id, node_str.data);
        output = string_append(&output, line_buffer, arena);
    }
    output = string_append(&output, "\n", arena);

    for (size_t i = 0; i < dg->nodes.size; ++i)
    {
        rg_node_t* n = DYN_ARRAY_GET(rg_node_t*, &dg->nodes, i);
        arena_dyn_array_t writer_edges = rg_dep_graph_get_writer_edges(dg, n, arena);

        char valid_str_buffer[1024] = "\0";
        char invalid_str_buffer[1024] = "\0";

        for (size_t j = 0; j < writer_edges.size; ++j)
        {
            rg_edge_t* edge = DYN_ARRAY_GET(rg_edge_t*, &writer_edges, j);
            rg_node_t* link = rg_dep_graph_get_node(dg, edge->to_id);
            if (rg_dep_graph_is_valid_edge(dg, edge))
            {
                if (valid_str_buffer[0] == '\0')
                {
                    sprintf(valid_str_buffer, "N%lu -> { N%lu ", n->id, link->id);
                }
                else
                {
                    sprintf(valid_str_buffer, "N%lu ", link->id);
                }
                output = string_append(&output, valid_str_buffer, arena);
            }
            else
            {
                if (invalid_str_buffer[0] == '\0')
                {
                    sprintf(invalid_str_buffer, "N%lu -> { N%lu ", n->id, link->id);
                }
                else
                {
                    sprintf(invalid_str_buffer, "N%lu ", link->id);
                }
                output = string_append(&output, invalid_str_buffer, arena);
            }
        }
        if (valid_str_buffer[0] != '\0')
        {
            sprintf(valid_str_buffer, "} [color=red4]\n");
            output = string_append(&output, valid_str_buffer, arena);
        }
        if (invalid_str_buffer[0] != '\0')
        {
            sprintf(invalid_str_buffer, "} [color=red4 style=dashed]\n");
            output = string_append(&output, invalid_str_buffer, arena);
        }
    }
    return string_append(&output, "}\n", arena);
}