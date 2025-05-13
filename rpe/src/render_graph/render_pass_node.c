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

#include "render_pass_node.h"

#include "render_graph.h"
#include "render_graph_pass.h"
#include "rendergraph_resource.h"
#include "resource_node.h"
#include "resources.h"
#include "vulkan-api/driver.h"

#include <string.h>
#include <utility/maths.h>

rg_pass_info_t rg_pass_info_init(const char* name, arena_t* arena)
{
    rg_pass_info_t i;
    memset(&i, 0, sizeof(rg_pass_info_t));
    i.name = string_init(name, arena);
    return i;
}

rg_pass_node_t* rg_pass_node_init(rg_dep_graph_t* dg, const char* name, arena_t* arena)
{
    assert(dg);
    rg_pass_node_t* node = ARENA_MAKE_STRUCT(arena, rg_pass_node_t, ARENA_ZERO_MEMORY);

    // Base bode init.
    node->base.name = string_init(name, arena);
    node->base.id = rg_dep_graph_create_id(dg);
    node->base.ref_count = 0;
    rg_dep_graph_add_node(dg, (rg_node_t*)node);

    MAKE_DYN_ARRAY(rg_resource_t*, arena, 30, &node->resources_to_bake);
    MAKE_DYN_ARRAY(rg_resource_t*, arena, 30, &node->resources_to_destroy);
    MAKE_DYN_ARRAY(rg_handle_t, arena, 30, &node->resource_handles);
    return node;
}

rg_render_pass_node_t*
rg_render_pass_node_init(rg_dep_graph_t* dg, const char* name, rg_pass_t* rg_pass, arena_t* arena)
{
    rg_render_pass_node_t* node =
        ARENA_MAKE_STRUCT(arena, rg_render_pass_node_t, ARENA_ZERO_MEMORY);

    // Base node init.
    node->base.base.name = string_init(name, arena);
    node->base.base.id = rg_dep_graph_create_id(dg);
    node->base.base.ref_count = 0;
    node->base.imported = false;
    rg_dep_graph_add_node(dg, (rg_node_t*)node);

    // Pass node init.
    MAKE_DYN_ARRAY(rg_resource_t*, arena, 30, &node->base.resources_to_bake);
    MAKE_DYN_ARRAY(rg_resource_t*, arena, 30, &node->base.resources_to_destroy);
    MAKE_DYN_ARRAY(rg_handle_t, arena, 30, &node->base.resource_handles);

    MAKE_DYN_ARRAY(rg_pass_info_t, arena, 30, &node->render_pass_targets);
    node->rg_pass = rg_pass;
    return node;
}

rg_present_pass_node_t*
rg_present_pass_node_init(rg_dep_graph_t* dg, const char* name, arena_t* arena)
{
    rg_present_pass_node_t* node =
        ARENA_MAKE_STRUCT(arena, rg_present_pass_node_t, ARENA_ZERO_MEMORY);

    // Base bode init.
    node->base.base.name = string_init(name, arena);
    node->base.base.id = rg_dep_graph_create_id(dg);
    node->base.base.ref_count = 0;
    node->base.imported = true;
    rg_dep_graph_add_node(dg, (rg_node_t*)node);

    MAKE_DYN_ARRAY(rg_resource_t*, arena, 30, &node->base.resources_to_bake);
    MAKE_DYN_ARRAY(rg_resource_t*, arena, 30, &node->base.resources_to_destroy);
    MAKE_DYN_ARRAY(rg_handle_t, arena, 30, &node->base.resource_handles);

    return node;
}

void rg_render_pass_info_bake(render_graph_t* rg, rg_pass_info_t* info, vkapi_driver_t* driver)
{
    assert(rg);
    assert(info);
    assert(driver);

    // Imported targets declare their own info so nothing to do here.
    if (info->imported)
    {
        return;
    }

    vkapi_attach_info_t col_info[VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT] = {0};
    for (size_t i = 0; i < VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT; ++i)
    {
        if (rg_handle_is_valid(info->desc.attachments.attach_array[i]))
        {
            rg_texture_resource_t* r =
                (rg_texture_resource_t*)rg_get_resource(rg, info->desc.attachments.attach_array[i]);
            col_info[i].handle = r->handle;

            // Now we have resolved the image usage - work out what to transition
            // to in the final layout of the renderpass.
            if ((r->image_usage & VK_IMAGE_USAGE_SAMPLED_BIT) ||
                (r->image_usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT))
            {
                info->vkapi_rpass_data.final_layouts[i] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            }
            else
            {
                // safe to assume that this is a colour attachment if not sampled/input??
                info->vkapi_rpass_data.final_layouts[i] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            }
        }
        else
        {
            col_info[i].handle.id = UINT32_MAX;
        }
    }

    vkapi_attach_info_t depthStencilInfo[2] = {0};
    for (size_t i = 0; i < 2; ++i)
    {
        rg_handle_t attachment =
            info->desc.attachments.attach_array[VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT + i];
        if (rg_handle_is_valid(attachment))
        {
            rg_texture_resource_t* r = (rg_texture_resource_t*)rg_get_resource(rg, attachment);
            depthStencilInfo[i].handle = r->handle;
            // TODO: also fill the layer and level from the subresource of the texture
        }
        else
        {
            depthStencilInfo[i].handle.id = UINT32_MAX;
        }
    }

    info->desc.rt_handle = vkapi_driver_create_rt(
        driver,
        info->desc.multi_view_count,
        info->desc.clear_col,
        col_info,
        depthStencilInfo[0],
        depthStencilInfo[1]);
}

void rg_pass_node_add_to_bake_list(rg_pass_node_t* node, rg_resource_t* r)
{
    assert(node);
    assert(r);
    DYN_ARRAY_APPEND(&node->resources_to_bake, &r);
}

void rg_pass_node_add_to_destroy_list(rg_pass_node_t* node, rg_resource_t* r)
{
    assert(node);
    assert(r);
    DYN_ARRAY_APPEND(&node->resources_to_destroy, &r);
}

void rg_pass_node_bake_resource_list(rg_pass_node_t* node, vkapi_driver_t* driver)
{
    assert(node);
    assert(driver);
    for (size_t i = 0; i < node->resources_to_bake.size; ++i)
    {
        rg_resource_t* r = DYN_ARRAY_GET(rg_resource_t*, &node->resources_to_bake, i);
        rg_resource_bake(r, driver);
    }
}

void rg_pass_node_destroy_resource_list(rg_pass_node_t* node, vkapi_driver_t* driver)
{
    assert(node);
    assert(driver);
    for (size_t i = 0; i < node->resources_to_destroy.size; ++i)
    {
        rg_resource_t* r = DYN_ARRAY_GET(rg_resource_t*, &node->resources_to_destroy, i);
        rg_resource_destroy(r, driver);
    }
}

void rg_pass_node_add_resource(rg_pass_node_t* node, render_graph_t* rg, rg_handle_t handle)
{
    assert(node);
    assert(rg);
    assert(rg_handle_is_valid(handle));

    rg_resource_t* r = rg_get_resource(rg, handle);
    rg_resource_register_pass(r, node);
    DYN_ARRAY_APPEND(&node->resource_handles, &handle);
}

rg_handle_t rg_rpass_node_create_rt(
    rg_render_pass_node_t* node, render_graph_t* rg, const char* name, rg_pass_desc_t desc)
{
    rg_pass_info_t info = rg_pass_info_init(name, rg_get_arena(rg));

    arena_dyn_array_t readers =
        rg_dep_graph_get_reader_edges(rg_get_dep_graph(rg), (rg_node_t*)node, rg_get_arena(rg));

    for (size_t i = 0; i < VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT; ++i)
    {
        if (rg_handle_is_valid(desc.attachments.attach_array[i]))
        {
            info.desc = desc;

            // Get any resources that read or write to this pass.
            rg_handle_t handle = desc.attachments.attach_array[i];
            for (size_t j = 0; j < readers.size; ++j)
            {
                rg_edge_t* r_edge = DYN_ARRAY_GET(rg_edge_t*, &readers, j);
                rg_resource_node_t* r_node = (rg_resource_node_t*)rg_dep_graph_get_node(
                    rg_get_dep_graph(rg), r_edge->from_id);
                assert(r_node);
                if (r_node->resource.id == handle.id)
                {
                    info.readers[i] = r_node;
                }
            }
            info.writers[i] = rg_get_resource_node(rg, handle);
            if (info.writers[i] == info.readers[i])
            {
                info.writers[i] = NULL;
            }
        }
    }
    rg_handle_t handle = {.id = node->render_pass_targets.size};
    DYN_ARRAY_APPEND(&node->render_pass_targets, &info);
    return handle;
}

void rg_render_pass_node_build(rg_render_pass_node_t* node, render_graph_t* rg)
{
    uint32_t min_width = UINT32_MAX;
    uint32_t min_height = UINT32_MAX;
    uint32_t max_width = 0;
    uint32_t max_height = 0;

    for (size_t i = 0; i < node->render_pass_targets.size; ++i)
    {
        rg_pass_info_t* info = DYN_ARRAY_GET_PTR(rg_pass_info_t, &node->render_pass_targets, i);
        rg_import_render_target_t* i_target = NULL;
        vkapi_render_pass_data_t* pass_data = &info->vkapi_rpass_data;

        for (size_t j = 0; j < VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT; ++j)
        {
            rg_handle_t attachment = info->desc.attachments.attach_array[j];

            pass_data->load_clear_flags[j] = RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_DONTCARE;
            pass_data->store_clear_flags[j] = RPE_BACKEND_RENDERPASS_STORE_CLEAR_FLAG_STORE;

            if (rg_handle_is_valid(attachment))
            {
                // for depth and stencil clear flags, use the manual settings
                // from the pass setup
                if (j == VKAPI_RENDER_TARGET_DEPTH_INDEX - 1)
                {
                    pass_data->load_clear_flags[j] = info->desc.ds_load_clear_flags[0];
                    pass_data->store_clear_flags[j] = info->desc.ds_store_clear_flags[0];
                }
                else if (j == VKAPI_RENDER_TARGET_STENCIL_INDEX - 1)
                {
                    pass_data->load_clear_flags[j] = info->desc.ds_load_clear_flags[1];
                    pass_data->store_clear_flags[j] = info->desc.ds_store_clear_flags[1];
                }
                else
                {
                    // if the pass has no readers then we can clear the store.
                    if (info->writers[j] && !rg_res_node_has_readers(info->writers[j]))
                    {
                        pass_data->store_clear_flags[j] =
                            RPE_BACKEND_RENDERPASS_STORE_CLEAR_FLAG_DONTCARE;
                    }
                    // if the pass has no writers then we can clear the load op.
                    if (!info->readers[j] ||
                        !rg_res_node_has_writers(
                            info->readers[j], rg_get_dep_graph(rg), rg_get_arena(rg)))
                    {
                        pass_data->load_clear_flags[j] =
                            RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_CLEAR;
                    }
                }

                // Work out the min and max width/height resource size across
                // all targets.
                rg_texture_resource_t* r = (rg_texture_resource_t*)rg_get_resource(rg, attachment);
                uint32_t t_width = r->desc.width;
                uint32_t t_height = r->desc.height;
                min_width = MIN(min_width, t_width);
                min_height = MIN(min_height, t_height);
                max_width = MAX(max_width, t_width);
                max_height = MAX(max_height, t_height);

                i_target = i_target != NULL ? i_target
                    : r->base.type == RG_RESOURCE_TYPE_IMPORTED_RENDER_TARGET
                    ? (rg_import_render_target_t*)r
                    : NULL;
            }
        }

        pass_data->clear_col = info->desc.clear_col;
        pass_data->width = max_width;
        pass_data->height = max_height;

        // If imported render target, overwrite the render pass data with the
        // imported parameters.
        if (i_target)
        {
            pass_data->clear_col = i_target->desc.clear_col;
            pass_data->width = i_target->desc.width;
            pass_data->height = i_target->desc.height;
            memcpy(
                pass_data->final_layouts,
                i_target->desc.final_layouts,
                sizeof(VkImageLayout) * VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT);
            memcpy(
                pass_data->init_layouts,
                i_target->desc.init_layouts,
                sizeof(VkImageLayout) * VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT);
            info->desc.rt_handle = i_target->rt_handle;
            info->imported = true;

            for (size_t j = 0; j < VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT; ++j)
            {
                if (pass_data->final_layouts[j] != VK_IMAGE_LAYOUT_UNDEFINED)
                {
                    pass_data->load_clear_flags[j] = i_target->desc.load_clear_flags[j];
                    pass_data->store_clear_flags[j] = i_target->desc.store_clear_flags[j];
                }
                else
                {
                    pass_data->load_clear_flags[j] =
                        RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_DONTCARE;
                    pass_data->store_clear_flags[j] =
                        RPE_BACKEND_RENDERPASS_STORE_CLEAR_FLAG_DONTCARE;
                }
            }
        }
    }
}

void rg_render_pass_node_execute(
    rg_render_pass_node_t* node,
    render_graph_t* rg,
    vkapi_driver_t* driver,
    rpe_engine_t* engine,
    rg_render_graph_resource_t* r)
{
    for (size_t i = 0; i < node->render_pass_targets.size; ++i)
    {
        rg_pass_info_t* info = DYN_ARRAY_GET_PTR(rg_pass_info_t, &node->render_pass_targets, i);
        rg_render_pass_info_bake(rg, info, driver);
    }

    node->rg_pass->func(driver, engine, r, node->rg_pass->data);
    // TODO: and delete the targets.
}

rg_pass_info_t rg_render_pass_node_get_rt_info(rg_render_pass_node_t* node, rg_handle_t handle)
{
    assert(handle.id < node->render_pass_targets.size && "Error whilst getting render target info");
    return DYN_ARRAY_GET(rg_pass_info_t, &node->render_pass_targets, handle.id);
}
