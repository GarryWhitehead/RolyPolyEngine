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

#include "resource_node.h"

#include "dependency_graph.h"
#include "render_graph.h"
#include "render_pass_node.h"
#include "resources.h"

#include <assert.h>

rg_resource_edge_t* rg_resource_edge_init(
    rg_dep_graph_t* dg, rg_node_t* from, rg_node_t* to, VkImageUsageFlags usage, arena_t* arena)
{
    assert(dg);
    assert(from);
    assert(to);
    rg_resource_edge_t* i = ARENA_MAKE_STRUCT(arena, rg_resource_edge_t, ARENA_ZERO_MEMORY);
    i->base = *rg_edge_init(dg, from, to, arena);
    i->usage = usage;
    return i;
}

rg_resource_node_t*
rg_res_node_init(rg_dep_graph_t* dg, const char* name, arena_t* arena, rg_handle_t* parent)
{
    assert(dg);
    rg_resource_node_t* i = ARENA_MAKE_STRUCT(arena, rg_resource_node_t, ARENA_ZERO_MEMORY);
    i->base = *rg_node_init(dg, name, arena);
    i->parent.id = parent == NULL ? UINT32_MAX : parent->id;
    MAKE_DYN_ARRAY(rg_resource_edge_t*, arena, 30, &i->reader_passes);
    MAKE_DYN_ARRAY(rg_resource_t*, arena, 30, &i->resources_to_bake);
    MAKE_DYN_ARRAY(rg_resource_t*, arena, 30, &i->resources_to_destroy);
    return i;
}

rg_resource_edge_t* rg_res_node_get_writer_edge(rg_resource_node_t* rn, rg_pass_node_t* node)
{
    assert(rn);
    assert(node);
    // If a writer pass has been registered with this node and its the
    // same node, then return the edge.
    if (rn->writer_pass && rn->writer_pass->base.from_id == node->base.id)
    {
        return rn->writer_pass;
    }
    return NULL;
}

void rg_res_node_set_writer_edge(rg_resource_node_t* rn, rg_resource_edge_t* edge)
{
    assert(rn);
    assert(edge);
    assert(!rn->writer_pass && "Only one writer per resource allowed.");
    rn->writer_pass = edge;
}

rg_resource_edge_t* rg_res_node_get_reader_edge(rg_resource_node_t* rn, rg_pass_node_t* node)
{
    assert(rn);
    assert(node);
    for (size_t i = 0; i < rn->reader_passes.size; ++i)
    {
        rg_resource_edge_t* edge = DYN_ARRAY_GET(rg_resource_edge_t*, &rn->reader_passes, i);
        if (edge->base.to_id == node->base.id)
        {
            return edge;
        }
    }
    return NULL;
}

void rg_res_node_set_reader_edge(rg_resource_node_t* rn, rg_resource_edge_t* edge)
{
    assert(rn);
    assert(edge);
    DYN_ARRAY_APPEND(&rn->reader_passes, &edge);
}

bool rg_res_node_set_parent_reader(
    rg_dep_graph_t* dg, rg_resource_node_t* rn, rg_resource_node_t* parent, arena_t* arena)
{
    assert(rn);
    assert(parent);
    if (!rn->parent_read_edge)
    {
        rn->parent_read_edge = rg_edge_init(dg, (rg_node_t*)rn, (rg_node_t*)parent, arena);
        return true;
    }
    return false;
}

bool rg_res_node_set_parent_writer(
    rg_dep_graph_t* dg, rg_resource_node_t* rn, rg_resource_node_t* parent, arena_t* arena)
{
    assert(rn);
    assert(parent);
    if (!rn->parent_write_edge)
    {
        rn->parent_write_edge = rg_edge_init(dg, (rg_node_t*)rn, (rg_node_t*)parent, arena);
        return true;
    }
    return false;
}

bool rg_res_node_has_writer_pass(rg_resource_node_t* rn)
{
    assert(rn);
    return rn->writer_pass;
}

bool rg_res_node_has_readers(rg_resource_node_t* rn)
{
    assert(rn);
    return rn->reader_passes.size > 0;
}

bool rg_res_node_has_writers(rg_resource_node_t* rn, rg_dep_graph_t* dg, arena_t* arena)
{
    assert(rn);
    assert(dg);
    return rg_dep_graph_get_writer_edges(dg, (rg_node_t*)rn, arena).size > 0;
}

void rg_res_node_add_resource_to_bake(rg_resource_node_t* rn, rg_resource_t* r)
{
    assert(rn);
    assert(r);
    DYN_ARRAY_APPEND(&rn->resources_to_bake, &r);
}

void rg_res_node_add_resource_to_destroy(rg_resource_node_t* rn, rg_resource_t* r)
{
    assert(rn);
    assert(r);
    DYN_ARRAY_APPEND(&rn->resources_to_destroy, &r);
}

void rg_res_node_bake_resources(rg_resource_node_t* rn, vkapi_driver_t* driver)
{
    assert(rn);
    assert(driver);
    for (size_t i = 0; i < rn->resources_to_bake.size; ++i)
    {
        rg_resource_t* r = DYN_ARRAY_GET(rg_resource_t*, &rn->resources_to_bake, i);
        rg_resource_bake(r, driver);
    }
}

void rg_res_node_destroy_resources(rg_resource_node_t* rn, vkapi_driver_t* driver)
{
    assert(rn);
    assert(driver);
    for (size_t i = 0; i < rn->resources_to_destroy.size; ++i)
    {
        rg_resource_t* r = DYN_ARRAY_GET(rg_resource_t*, &rn->resources_to_destroy, i);
        rg_resource_destroy(r, driver);
    }
}

void rg_res_node_update_res_usage(rg_resource_node_t* rn, render_graph_t* rg, rg_dep_graph_t* dg)
{
    assert(rn);
    assert(dg);
    rg_resource_t* r = rg_get_resource(rg, rn->resource);
    rg_tex_resource_update_res_usage(
        dg, (rg_texture_resource_t*)r, &rn->reader_passes, rn->writer_pass);
}

rg_resource_node_t* rg_res_node_get_parent_node(rg_resource_node_t* rn, render_graph_t* rg)
{
    assert(rn);
    assert(rg);
    if (rg_handle_is_valid(rn->parent))
    {
        return rg_get_resource_node(rg, rn->parent);
    }
    return NULL;
}

void rg_res_node_set_alias_res_edge(
    rg_dep_graph_t* dg, rg_resource_node_t* rn, rg_resource_node_t* alias, arena_t* arena)
{
    assert(rn);
    assert(alias);
    rn->alias_edge = rg_edge_init(dg, (rg_node_t*)rn, (rg_node_t*)alias, arena);
}
