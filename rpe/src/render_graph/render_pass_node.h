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

#ifndef __RPE_RG_RENDER_PASS_NODE_H__
#define __RPE_RG_RENDER_PASS_NODE_H__

#include "dependency_graph.h"
#include "render_graph_handle.h"
#include "render_graph_pass.h"
#include "utility/string.h"
#include "vulkan-api/renderpass.h"


// forward declarations
typedef struct ResourceNode rg_resource_node_t;
typedef struct RenderGraphPass rg_pass_t;
typedef struct VkApiDriver vkapi_driver_t;
typedef struct RenderGraph render_graph_t;
typedef struct Resource rg_resource_t;

/**
 * @brief All the information required to create a concrete vulkan renderpass
 */
typedef struct RenderPassInfo
{
    string_t name;
    rg_resource_node_t* readers[VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT];
    rg_resource_node_t* writers[VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT];
    rg_pass_desc_t desc;
    bool imported;
    /** Vulkan API backend **/
    vkapi_render_pass_data_t vkapi_rpass_data;
} rg_pass_info_t;

typedef struct PassNode
{
    rg_node_t base;
    bool imported;
    arena_dyn_array_t resources_to_bake;
    arena_dyn_array_t resources_to_destroy;
    arena_dyn_array_t resource_handles;
} rg_pass_node_t;

typedef struct RenderPassNode
{
    rg_pass_node_t base;
    rg_pass_t* rg_pass;
    arena_dyn_array_t render_pass_targets;
} rg_render_pass_node_t;

typedef struct PresentPassNode
{
    rg_pass_node_t base;
} rg_present_pass_node_t;

rg_pass_info_t rg_pass_info_init(const char* name, arena_t* arena);

rg_pass_node_t* rg_pass_node_init(rg_dep_graph_t* dg, const char* name, arena_t* arena);

rg_render_pass_node_t*
rg_render_pass_node_init(rg_dep_graph_t* dg, const char* name, rg_pass_t* rg_pass, arena_t* arena);

rg_present_pass_node_t*
rg_present_pass_node_init(rg_dep_graph_t* dg, const char* name, arena_t* arena);

void rg_pass_node_add_to_bake_list(rg_pass_node_t* node, rg_resource_t* r);

void rg_pass_node_add_to_destroy_list(rg_pass_node_t* node, rg_resource_t* r);

void rg_pass_node_bake_resource_list(rg_pass_node_t* node, vkapi_driver_t* driver);

void rg_pass_node_destroy_resource_list(rg_pass_node_t* node, vkapi_driver_t* driver);

void rg_pass_node_add_resource(rg_pass_node_t* node, render_graph_t* rg, rg_handle_t handle);

rg_handle_t rg_rpass_node_create_rt(
    rg_render_pass_node_t* node, render_graph_t* rg, const char* name, rg_pass_desc_t desc);

void rg_render_pass_node_build(rg_render_pass_node_t* node, render_graph_t* rg);

void rg_render_pass_node_execute(
    rg_render_pass_node_t* node,
    render_graph_t* rg,
    rg_pass_t* rp,
    vkapi_driver_t* driver,
    rpe_engine_t* engine,
    rg_render_graph_resource_t* r);

rg_pass_info_t rg_render_pass_node_get_rt_info(rg_render_pass_node_t* node, rg_handle_t handle);

#endif
