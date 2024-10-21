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

#ifndef __RPE_RG_RESOURCES_H__
#define __RPE_RG_RESOURCES_H__

#include "render_graph_handle.h"

#include <utility/arena.h>
#include <utility/maths.h>
#include <utility/string.h>
#include <vulkan-api/common.h>
#include <vulkan-api/renderpass.h>
#include <vulkan-api/resource_cache.h>


// forward declarations
typedef struct PassNode rg_pass_node_t;
typedef struct ResourceNode rg_resource_node_t;
typedef struct Resource rg_resource_t;
typedef struct VkApiDriver vkapi_driver_t;
typedef struct DependencyGraph rg_dep_graph_t;
typedef struct ResourceEdge rg_resource_edge_t;

enum ResourceType
{
    RG_RESOURCE_TYPE_TEXTURE,
    RG_RESOURCE_TYPE_IMPORTED,
    RG_RESOURCE_TYPE_IMPORTED_RENDER_TARGET,
    RG_RESOURCE_TYPE_NONE
};

typedef struct Resource
{
    /// For degugging purposes.
    string_t name;

    // ==== set by the compiler =====
    // The number of passes this resource is being used as an input.
    size_t read_count;

    rg_pass_node_t* first_pass_node;
    rg_pass_node_t* last_pass_node;

    arena_dyn_array_t writers;

    rg_resource_t* parent;
    enum ResourceType type;
    bool imported;

} rg_resource_t;

typedef struct TextureDescriptor
{
    uint32_t width;
    uint32_t height;
    uint8_t depth;
    uint8_t mip_levels;
    VkFormat format;
} rg_texture_desc_t;

// All the information needed to build a vulkan texture
typedef struct TextureResource
{
    rg_resource_t base;
    /// the image information which will be used to create the image view
    rg_texture_desc_t desc;
    /// this is resolved only upon calling render graph compile()
    VkImageUsageFlags image_usage;
    /// Only valid after call to "bake".
    /// Note: this will be invalid if resource is imported.
    texture_handle_t handle;
} rg_texture_resource_t;

// used for imported texture targets
typedef struct ImportedResource
{
    rg_texture_resource_t base;
} rg_imported_resource_t;

typedef struct ImportedRtDes
{
    enum LoadClearFlags load_clear_flags[VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT];
    enum StoreClearFlags store_clear_flags[VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT];

    VkImageLayout init_layouts[VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT];
    VkImageLayout final_layouts[VKAPI_RENDER_TARGET_MAX_ATTACH_COUNT];

    VkImageUsageFlags usage;
    math_vec4f clear_col;

    uint32_t width;
    uint32_t height;
    uint8_t samples;
} rg_import_rt_desc_t;

typedef struct ImportedRenderTarget
{
    rg_imported_resource_t base;
    // handle to the backend render target which will be imported into the graph
    vkapi_rt_handle_t rt_handle;
    rg_import_rt_desc_t desc;
} rg_import_render_target_t;

rg_resource_t* rg_resource_init(const char* name, enum ResourceType type, arena_t* arena);

rg_texture_resource_t* rg_tex_resource_init(
    const char* name, VkImageUsageFlags image_usage, rg_texture_desc_t desc, arena_t* arena);

rg_imported_resource_t* rg_import_resource_init(
    const char* name,
    VkImageUsageFlags image_usage,
    rg_texture_desc_t desc,
    texture_handle_t handle,
    arena_t* arena);

rg_import_render_target_t* rg_tex_import_rt_init(
    const char* name,
    VkImageUsageFlags image_usage,
    rg_texture_desc_t tex_desc,
    rg_import_rt_desc_t import_desc,
    vkapi_rt_handle_t rt_handle,
    arena_t* arena);

void rg_resource_register_pass(rg_resource_t* r, rg_pass_node_t* node);

bool rg_resource_connect_writer(
    rg_pass_node_t* pass_node,
    rg_dep_graph_t* dg,
    rg_resource_node_t* resource_node,
    VkImageUsageFlags usage,
    arena_t* arena);

bool rg_resource_connect_reader(
    rg_pass_node_t* pass_node,
    rg_dep_graph_t* dg,
    rg_resource_node_t* resource_node,
    VkImageUsageFlags usage,
    arena_t* arena);

void rg_resource_bake(rg_resource_t* r, vkapi_driver_t* driver);

void rg_resource_destroy(rg_resource_t* r, vkapi_driver_t* driver);

void rg_tex_resource_update_res_usage(
    rg_dep_graph_t* dg,
    rg_texture_resource_t* r,
    arena_dyn_array_t* edges,
    rg_resource_edge_t* writer);

bool rg_resource_is_sub_resource(rg_resource_t* r);

#endif
