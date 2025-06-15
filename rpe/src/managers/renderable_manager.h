/* Copyright (c) 2024-2025 Garry Whitehead
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

#ifndef __RPE_PRIV_RENDERABLE_MANAGER_H__
#define __RPE_PRIV_RENDERABLE_MANAGER_H__

#include "material.h"
#include "rpe/aabox.h"
#include "rpe/renderable_manager.h"

#include <backend/objects.h>
#include <utility/arena.h>
#include <vulkan-api/program_manager.h>

typedef struct Engine rpe_engine_t;
typedef struct ComponentManager rpe_comp_manager_t;
typedef struct Object rpe_object_t;
struct RenderableInstance;

typedef struct Mesh
{
    // Note: The offsets here are into the "uber" vertex/index buffer held by the resource cache.
    size_t index_count;
    uint32_t index_offset;
    uint32_t vertex_offset;
    enum MeshAttributeFlags mesh_flags;
} rpe_mesh_t;

typedef struct Renderable
{
    rpe_mesh_t* mesh_data;
    rpe_material_t* material;
    rpe_object_t transform_obj;
    // The extents of this primitive.
    rpe_aabox_t box;
    uint64_t sort_key;
    rpe_rect2d_t scissor;
    rpe_viewport_t viewport;
    uint8_t view_layer;
    // States whether frustum culling should be skipped for this renderable.
    bool perform_cull_test;

    // Used for the material key - batching is dependent on viewport/scissor changes.
    struct RenderableKey
    {
        rpe_rect2d_t scissor;
        rpe_viewport_t viewport;
    } key;

} rpe_renderable_t;

typedef struct BatchedDraw
{
    rpe_material_t* material;
    uint32_t first_idx;
    uint32_t count;
    rpe_rect2d_t scissor;
    rpe_viewport_t viewport;
} rpe_batch_renderable_t;

// clang-format off
struct IndirectDraw
{
    VkDrawIndexedIndirectCommand indirect_cmd;  // 20 bytes
    uint32_t object_id;                         // 4 bytes
    uint32_t batch_id;                          // 4 bytes
    bool shadow_caster;                         // 4 bytes
    bool perform_cull_test;                     // 4 bytes
    int padding;                                // 4 bytes
};                                              // Total : 40bytes.
// clang-format on

typedef struct RenderableManager
{
    rpe_engine_t* engine;
    arena_dyn_array_t renderables;
    arena_dyn_array_t materials;
    arena_dyn_array_t meshes;
    arena_dyn_array_t vertex_allocations;
    rpe_comp_manager_t* comp_manager;

} rpe_rend_manager_t;

rpe_renderable_t* rpe_renderable_init(arena_t* arena);

rpe_rend_manager_t* rpe_rend_manager_init(rpe_engine_t* engine, arena_t* arena);

rpe_renderable_t* rpe_rend_manager_get_mesh(rpe_rend_manager_t* m, rpe_object_t* obj);

void rpe_rend_manager_batch_renderables(
    rpe_rend_manager_t* m, struct RenderableInstance* instances, size_t count, arena_dyn_array_t* batched_renderables);

#endif
