/* Copyright (c) 2018-2020 Garry Whitehead
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

#include "resources.h"

#include "render_graph/dependency_graph.h"
#include "render_graph/render_pass_node.h"
#include "render_graph/resource_node.h"

#include <utility/string.h>
#include <vulkan-api/driver.h>
#include <vulkan-api/texture.h>

rg_resource_t* rg_resource_init(const char* name, enum ResourceType type, arena_t* arena)
{
    rg_resource_t* i = ARENA_MAKE_STRUCT(arena, rg_resource_t, ARENA_ZERO_MEMORY);
    i->name = string_init(name, arena);
    i->type = type;
    i->parent = i;
    return i;
}

void rg_resource_register_pass(rg_resource_t* r, rg_pass_node_t* node)
{
    assert(r);
    assert(node);
    r->read_count++;
    // Keep a record of the first and last passes to reference this resource.
    if (!r->first_pass_node)
    {
        r->first_pass_node = node;
    }
    r->last_pass_node = node;
}

bool rg_resource_connect_writer(
    rg_pass_node_t* pass_node,
    rg_dep_graph_t* dg,
    rg_resource_node_t* resource_node,
    VkImageUsageFlags usage,
    arena_t* arena)
{
    rg_resource_edge_t* edge = rg_res_node_get_writer_edge(resource_node, pass_node);
    if (edge)
    {
        edge->usage |= usage;
    }
    else
    {
        rg_resource_edge_t* new_edge = rg_resource_edge_init(
            dg, (rg_node_t*)pass_node, (rg_node_t*)resource_node, usage, arena);
        rg_res_node_set_writer_edge(resource_node, new_edge);
    }
    return true;
}

bool rg_resource_connect_reader(
    rg_pass_node_t* pass_node,
    rg_dep_graph_t* dg,
    rg_resource_node_t* resource_node,
    VkImageUsageFlags usage,
    arena_t* arena)
{
    rg_resource_edge_t* edge = rg_res_node_get_reader_edge(resource_node, pass_node);
    if (edge)
    {
        edge->usage |= usage;
    }
    else
    {
        rg_resource_edge_t* new_edge = rg_resource_edge_init(
            dg, (rg_node_t*)pass_node, (rg_node_t*)resource_node, usage, arena);
        rg_res_node_set_reader_edge(resource_node, new_edge);
    }
    return true;
}

bool rg_resource_is_sub_resource(rg_resource_t* r)
{
    assert(r);
    return r->parent != r;
}

rg_texture_resource_t* rg_tex_resource_init(
    const char* name, VkImageUsageFlags image_usage, rg_texture_desc_t desc, arena_t* arena)
{
    rg_texture_resource_t* i = ARENA_MAKE_STRUCT(arena, rg_texture_resource_t, ARENA_ZERO_MEMORY);
    i->base.name = string_init(name, arena);
    i->base.type = RG_RESOURCE_TYPE_TEXTURE;
    i->base.parent = (rg_resource_t *)i;
    i->image_usage = image_usage;
    i->desc = desc;
    i->base.imported = false;
    return i;
}

void rg_tex_resource_update_res_usage(
    rg_dep_graph_t* dg,
    rg_texture_resource_t* r,
    arena_dyn_array_t* edges,
    rg_resource_edge_t* writer)
{
    assert(r);
    assert(dg);
    for (size_t i = 0; i < edges->size; ++i)
    {
        rg_resource_edge_t* edge = DYN_ARRAY_GET(rg_resource_edge_t*, edges, i);
        if (rg_dep_graph_is_valid_edge(dg, (rg_edge_t*)edge))
        {
            r->image_usage |= edge->usage;
        }
    }
    if (writer)
    {
        r->image_usage |= writer->usage;
    }
    // Also propagate the image usage flags to any sub-resources.
    rg_texture_resource_t* curr = r;
    while (curr != (rg_texture_resource_t*)curr->base.parent)
    {
        curr = (rg_texture_resource_t*)curr->base.parent;
        curr->image_usage = r->image_usage;
    }
}

void rg_resource_bake(rg_resource_t* r, vkapi_driver_t* driver)
{
    assert(r);
    assert(driver);
    switch (r->type)
    {
        case RG_RESOURCE_TYPE_TEXTURE: {
            rg_texture_resource_t* r_tex = (rg_texture_resource_t*)r;
            r_tex->handle = vkapi_driver_create_tex2d(
                driver,
                r_tex->desc.format,
                r_tex->desc.width,
                r_tex->desc.height,
                r_tex->desc.mip_levels,
                1,
                1,
                r_tex->image_usage);
            break;
        }
        case RG_RESOURCE_TYPE_IMPORTED:
        case RG_RESOURCE_TYPE_IMPORTED_RENDER_TARGET:
        case RG_RESOURCE_TYPE_NONE:
            break;
    }
}

void rg_resource_destroy(rg_resource_t* r, vkapi_driver_t* driver)
{
    assert(r);
    assert(driver);
    switch (r->type)
    {
        case RG_RESOURCE_TYPE_TEXTURE: {
            rg_texture_resource_t* r_tex = (rg_texture_resource_t*)r;
            vkapi_driver_destroy_tex2d(driver, r_tex->handle);
        }
        case RG_RESOURCE_TYPE_IMPORTED:
        case RG_RESOURCE_TYPE_IMPORTED_RENDER_TARGET:
        case RG_RESOURCE_TYPE_NONE:
            break;
    }
}

rg_imported_resource_t* rg_import_resource_init(
    const char* name,
    VkImageUsageFlags image_usage,
    rg_texture_desc_t desc,
    texture_handle_t handle,
    arena_t* arena)
{
    rg_imported_resource_t* i = ARENA_MAKE_STRUCT(arena, rg_imported_resource_t, ARENA_ZERO_MEMORY);
    i->base.base.name = string_init(name, arena);
    i->base.base.type = RG_RESOURCE_TYPE_IMPORTED;
    i->base.base.parent = (rg_resource_t*)i;
    i->base.image_usage = image_usage;
    i->base.desc = desc;
    i->base.base.imported = true;
    i->base.handle = handle;
    return i;
}

rg_import_render_target_t* rg_tex_import_rt_init(
    const char* name,
    VkImageUsageFlags image_usage,
    rg_texture_desc_t tex_desc,
    rg_import_rt_desc_t import_desc,
    vkapi_rt_handle_t rt_handle,
    arena_t* arena)
{
    rg_import_render_target_t* i =
        ARENA_MAKE_STRUCT(arena, rg_import_render_target_t, ARENA_ZERO_MEMORY);
    texture_handle_t handle = {.id = UINT32_MAX};
    i->base.base.base.name = string_init(name, arena);
    i->base.base.base.type = RG_RESOURCE_TYPE_IMPORTED_RENDER_TARGET;
    i->base.base.base.parent = (rg_resource_t*)i;
    i->base.base.image_usage = image_usage;
    i->base.base.desc = tex_desc;
    i->base.base.base.imported = true;
    i->base.base.handle = handle;
    i->desc = import_desc;
    i->rt_handle = rt_handle;
    return i;
}
