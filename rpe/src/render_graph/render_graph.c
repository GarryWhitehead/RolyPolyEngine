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

#include "render_graph.h"

#include "backboard.h"
#include "dependency_graph.h"
#include "render_graph_pass.h"
#include "render_pass_node.h"
#include "rendergraph_resource.h"
#include "resource_node.h"
#include "resources.h"

#include <utility/hash.h>
#include <vulkan-api/driver.h>
#include <vulkan-api/renderpass.h>

#include <string.h>

render_graph_t* rg_init(arena_t* arena)
{
    render_graph_t* rg = ARENA_MAKE_STRUCT(arena, render_graph_t, ARENA_ZERO_MEMORY);
    MAKE_DYN_ARRAY(rg_resource_slot_t, arena, 40, &rg->resource_slots);
    MAKE_DYN_ARRAY(rg_resource_t*, arena, 20, &rg->resources);
    MAKE_DYN_ARRAY(rg_resource_node_t*, arena, 20, &rg->resource_nodes);
    MAKE_DYN_ARRAY(rg_render_pass_node_t*, arena, 20, &rg->pass_nodes);
    MAKE_DYN_ARRAY(rg_pass_t*, arena, 20, &rg->rg_passes);
    rg->backboard = rg_backboard_init(arena);
    rg->dep_graph = rg_dep_graph_init(arena);
    rg->arena = arena;
    return rg;
}

rg_render_pass_node_t* rg_create_pass_node(render_graph_t* rg, const char* name, rg_pass_t* rg_pass)
{
    assert(rg);
    assert(rg_pass);
    rg_pass->node = rg_render_pass_node_init(rg->dep_graph, name, rg_pass, rg->arena);
    DYN_ARRAY_APPEND(&rg->pass_nodes, &rg_pass->node);
    return rg_pass->node;
}

void rg_add_present_pass(render_graph_t* rg, rg_handle_t handle)
{
    assert(rg);
    rg_present_pass_node_t* node =
        rg_present_pass_node_init(rg->dep_graph, "PresentPass", rg->arena);
    rg_add_read(rg, handle, (rg_pass_node_t*)node, 0);
    rg_node_declare_side_effect((rg_node_t*)node);
    DYN_ARRAY_APPEND(&rg->pass_nodes, &node);
}

rg_handle_t rg_add_resource(render_graph_t* rg, rg_resource_t* r, rg_handle_t* parent)
{
    assert(rg);
    assert(r);

    rg_handle_t handle = {.id = rg->resource_slots.size};
    rg_resource_slot_t slot = {
        .node_idx = rg->resource_nodes.size, .resource_idx = rg->resources.size};
    DYN_ARRAY_APPEND(&rg->resource_slots, &slot);
    rg_resource_node_t* r_node =
        rg_res_node_init(rg->dep_graph, r->name.data, rg->arena, handle, parent);
    DYN_ARRAY_APPEND(&rg->resources, &r);
    DYN_ARRAY_APPEND(&rg->resource_nodes, &r_node);
    return handle;
}

rg_handle_t rg_move_resource(render_graph_t* rg, rg_handle_t from, rg_handle_t to)
{
    assert(rg_handle_is_valid(from));
    assert(rg_handle_is_valid(to));

    rg_resource_slot_t* from_slot =
        DYN_ARRAY_GET_PTR(rg_resource_slot_t, &rg->resource_slots, from.id);
    rg_resource_slot_t* to_slot = DYN_ARRAY_GET_PTR(rg_resource_slot_t, &rg->resource_slots, to.id);
    rg_resource_node_t* from_node = rg_get_resource_node(rg, from);
    rg_resource_node_t* to_node = rg_get_resource_node(rg, to);

    // Connect the replacement node to the forwarded node.
    rg_res_node_set_alias_res_edge(rg->dep_graph, from_node, to_node, rg->arena);
    from_slot->resource_idx = to_slot->resource_idx;
    return from;
}

rg_resource_node_t* rg_get_resource_node(render_graph_t* rg, rg_handle_t handle)
{
    assert(rg);
    assert(handle.id < rg->resource_slots.size);
    rg_resource_slot_t slot = DYN_ARRAY_GET(rg_resource_slot_t, &rg->resource_slots, handle.id);
    return DYN_ARRAY_GET(rg_resource_node_t*, &rg->resource_nodes, slot.node_idx);
}

rg_handle_t rg_import_render_target(
    render_graph_t* rg, const char* name, rg_import_rt_desc_t desc, vkapi_rt_handle_t handle)
{
    rg_texture_desc_t r_desc = {.width = desc.width, .height = desc.height};
    // TODO: What should image usage be here?
    rg_import_render_target_t* i = rg_tex_import_rt_init(name, 0, r_desc, desc, handle, rg->arena);
    return rg_add_resource(rg, (rg_resource_t*)i, NULL);
}

rg_handle_t rg_add_read(
    render_graph_t* rg, rg_handle_t handle, rg_pass_node_t* pass_node, VkImageUsageFlags usage)
{
    assert(rg);
    assert(rg_handle_is_valid(handle));
    assert(pass_node);

    assert(handle.id < rg->resources.size);
    rg_resource_t* r = DYN_ARRAY_GET(rg_resource_t*, &rg->resources, handle.id);

    assert(handle.id < rg->resource_nodes.size);
    rg_resource_node_t* node = DYN_ARRAY_GET(rg_resource_node_t*, &rg->resource_nodes, handle.id);

    rg_resource_connect_reader(pass_node, rg->dep_graph, node, usage, rg->arena);
    if (rg_resource_is_sub_resource(r))
    {
        // if this is a subresource, it has a write dependency
        rg_resource_node_t* parent_node = rg_res_node_get_parent_node(node, rg);
        rg_res_node_set_parent_writer(rg->dep_graph, node, parent_node, rg->arena);
    }
    return handle;
}

rg_handle_t rg_add_write(
    render_graph_t* rg, rg_handle_t handle, rg_pass_node_t* pass_node, VkImageUsageFlags usage)
{
    assert(rg);
    assert(rg_handle_is_valid(handle));
    assert(pass_node);

    assert(handle.id < rg->resources.size);
    rg_resource_t* r = DYN_ARRAY_GET(rg_resource_t*, &rg->resources, handle.id);

    assert(handle.id < rg->resource_nodes.size);
    rg_resource_node_t* node = DYN_ARRAY_GET(rg_resource_node_t*, &rg->resource_nodes, handle.id);

    rg_resource_connect_writer(pass_node, rg->dep_graph, node, usage, rg->arena);

    // If it's an imported resource, make sure the pass node its writing to is not culled.
    if (r->imported)
    {
        rg_node_declare_side_effect((rg_node_t*)pass_node);
    }

    if (rg_resource_is_sub_resource(r))
    {
        // If this is a subresource, it has a write dependency.
        rg_resource_node_t* parent = rg_res_node_get_parent_node(node, rg);
        rg_res_node_set_parent_writer(rg->dep_graph, node, parent, rg->arena);
    }
    return handle;
}

render_graph_t* rg_compile(render_graph_t* rg)
{
    assert(rg);

    rg_dep_graph_cull(rg->dep_graph, rg->arena);

    size_t tmp_idx = 0;
    rg->active_idx = 0;
    rg_pass_node_t** tmp_buffer =
        ARENA_MAKE_ARRAY(rg->arena, rg_pass_node_t*, rg->pass_nodes.size, 0);
    for (size_t i = 0; i < rg->pass_nodes.size; ++i)
    {
        rg_pass_node_t* node = DYN_ARRAY_GET(rg_pass_node_t*, &rg->pass_nodes, i);
        if (!rg_node_is_culled((rg_node_t*)node))
        {
            DYN_ARRAY_SET(&rg->pass_nodes, rg->active_idx++, &node);
        }
        else
        {
            tmp_buffer[tmp_idx++] = node;
        }
    }
    assert(rg->active_idx + tmp_idx == rg->pass_nodes.size);
    uint8_t* node_ptr = (uint8_t*)rg->pass_nodes.data + rg->active_idx * sizeof(rg_pass_node_t*);
    memcpy(node_ptr, tmp_buffer, tmp_idx * sizeof(rg_pass_node_t*));

    size_t node_idx = 0;

    while (node_idx < rg->active_idx)
    {
        assert(node_idx < rg->pass_nodes.size);
        rg_pass_node_t* pass_node = DYN_ARRAY_GET(rg_pass_node_t*, &rg->pass_nodes, node_idx);

        arena_dyn_array_t readers =
            rg_dep_graph_get_reader_edges(rg->dep_graph, (rg_node_t*)pass_node, rg->arena);
        for (size_t i = 0; i < readers.size; ++i)
        {
            rg_edge_t* edge = DYN_ARRAY_GET(rg_edge_t*, &readers, i);
            rg_resource_node_t* r_node =
                (rg_resource_node_t*)rg_dep_graph_get_node(rg->dep_graph, edge->from_id);
            rg_pass_node_add_resource(pass_node, rg, r_node->resource);
        }

        arena_dyn_array_t writers =
            rg_dep_graph_get_writer_edges(rg->dep_graph, (rg_node_t*)pass_node, rg->arena);
        for (size_t i = 0; i < writers.size; ++i)
        {
            rg_edge_t* edge = DYN_ARRAY_GET(rg_edge_t*, &writers, i);
            rg_resource_node_t* r_node =
                (rg_resource_node_t*)rg_dep_graph_get_node(rg->dep_graph, edge->to_id);
            rg_pass_node_add_resource(pass_node, rg, r_node->resource);
        }

        if (!pass_node->imported)
        {
            rg_render_pass_node_build(pass_node, rg);
        }
        ++node_idx;
    }

    // Bake the resources.
    for (size_t i = 0; i < rg->resources.size; ++i)
    {
        rg_resource_t* r = DYN_ARRAY_GET(rg_resource_t*, &rg->resources, i);
        if (r->read_count > 0)
        {
            rg_pass_node_t* first = r->first_pass_node;
            rg_pass_node_t* last = r->last_pass_node;

            if (first && last)
            {
                rg_pass_node_add_to_bake_list(first, r);
                rg_pass_node_add_to_destroy_list(last, r);
            }
        }
    }

    // Update the usage flags for all resources.
    for (size_t i = 0; i < rg->resource_nodes.size; ++i)
    {
        rg_resource_node_t* node = DYN_ARRAY_GET(rg_resource_node_t*, &rg->resource_nodes, i);
        rg_res_node_update_res_usage(node, rg, rg->dep_graph);
    }

    return rg;
}

void rg_execute(render_graph_t* rg, vkapi_driver_t* driver, rpe_engine_t* engine)
{
    assert(rg);
    assert(driver);
    size_t node_idx = 0;

    while (node_idx < rg->active_idx)
    {
        assert(node_idx < rg->pass_nodes.size);
        rg_render_pass_node_t* pass_node =
            DYN_ARRAY_GET(rg_render_pass_node_t*, &rg->pass_nodes, node_idx++);

        // Create concrete vulkan resources - these are added to the
        // node during the compile call.
        rg_pass_node_bake_resource_list((rg_pass_node_t*)pass_node, driver);

        if (!pass_node->base.imported)
        {
            rg_render_graph_resource_t r = {.rg = rg, .pass_node = pass_node};
            rg_render_pass_node_execute(pass_node, rg, driver, engine, &r);
        }

        rg_pass_node_destroy_resource_list((rg_pass_node_t*)pass_node, driver);
    }
}

rg_resource_t* rg_get_resource(render_graph_t* rg, rg_handle_t handle)
{
    assert(rg);
    assert(handle.id < rg->resource_slots.size);
    rg_resource_slot_t slot = DYN_ARRAY_GET(rg_resource_slot_t, &rg->resource_slots, handle.id);
    assert(slot.resource_idx < rg->resources.size);
    return DYN_ARRAY_GET(rg_resource_t*, &rg->resources, slot.resource_idx);
}

rg_pass_t* rg_add_pass(
    render_graph_t* rg,
    const char* name,
    setup_func setup,
    execute_func execute,
    size_t data_size,
    void* local_data)
{
    rg_pass_t* pass = rg_pass_init(execute, data_size, rg->arena);
    rg_render_pass_node_t* node = rg_create_pass_node(rg, name, pass);
    setup(rg, (rg_pass_node_t*)node, pass->data, local_data);
    DYN_ARRAY_APPEND(&rg->rg_passes, &pass);
    return pass;
}

void _executor_setup(render_graph_t* rg, rg_pass_node_t* node, void* data, void* local)
{
    rg_node_declare_side_effect((rg_node_t*)node);
}

void rg_add_executor_pass(render_graph_t* rg, const char* name, execute_func execute)
{
    rg_add_pass(rg, name, _executor_setup, execute, 0, NULL);
}

void rg_clear(render_graph_t* rg)
{
    dyn_array_clear(&rg->resources);
    dyn_array_clear(&rg->pass_nodes);
    dyn_array_clear(&rg->resource_slots);
    dyn_array_clear(&rg->resource_nodes);
    dyn_array_clear(&rg->rg_passes);
    rg_backboard_reset(&rg->backboard);
    rg_dep_graph_clear(rg->dep_graph);
}

arena_t* rg_get_arena(render_graph_t* rg)
{
    assert(rg);
    return rg->arena;
}

rg_dep_graph_t* rg_get_dep_graph(render_graph_t* rg)
{
    assert(rg);
    return rg->dep_graph;
}

rg_backboard_t* rg_get_backboard(render_graph_t* rg)
{
    assert(rg);
    return &rg->backboard;
}
