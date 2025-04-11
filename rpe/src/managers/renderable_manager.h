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

#include <utility/arena.h>
#include <vulkan-api/program_manager.h>

typedef struct Engine rpe_engine_t;
typedef struct ComponentManager rpe_comp_manager_t;
typedef struct Object rpe_object_t;

enum MeshAttributeFlags
{
    RPE_MESH_ATTRIBUTE_POSITION = 1 << 0,
    RPE_MESH_ATTRIBUTE_UV0 = 1 << 1,
    RPE_MESH_ATTRIBUTE_UV1 = 1 << 2,
    RPE_MESH_ATTRIBUTE_NORMAL = 1 << 3,
    RPE_MESH_ATTRIBUTE_TANGENT = 1 << 4,
    RPE_MESH_ATTRIBUTE_COLOUR = 1 << 5,
    RPE_MESH_ATTRIBUTE_BONE_WEIGHT = 1 << 6,
    RPE_MESH_ATTRIBUTE_BONE_ID = 1 << 7
};

// Note: On linux (not sure about windows) we get an extra 8bytes of packing, so disable otherwise
// messes up attribute strides on the shader.
RPE_PACKED(typedef struct Vertex
{
    math_vec3f position;
    math_vec3f normal;
    math_vec2f uv0;
    math_vec2f uv1;
    math_vec4f tangent;
    math_vec4f colour;
    math_vec4f bone_weight;
    math_vec4f bone_id;
}) rpe_vertex_t;

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
    //VkPrimitiveTopology topology;
    //VkBool32 prim_restart;
    rpe_mesh_t* mesh_data;
    rpe_material_t* material;
    //uint64_t material_flags;
    rpe_object_t transform_obj;

    // The extents of this primitive.
    rpe_aabox_t box;
    uint64_t sort_key;
} rpe_renderable_t;

typedef struct BatchedDraw
{
    rpe_material_t* material;
    uint32_t first_idx;
    uint32_t count;
} rpe_batch_renderable_t;

struct IndirectDraw
{
    VkDrawIndexedIndirectCommand indirect_cmd;
    uint32_t object_id;
    uint32_t batch_id;
    bool shadow_caster;
};

typedef struct RenderableManager
{
    rpe_engine_t* engine;

    arena_dyn_array_t batched_renderables;

    // A renderable is a combination of mesh and material data.
    arena_dyn_array_t renderables;

    arena_dyn_array_t materials;
    arena_dyn_array_t meshes;

    rpe_comp_manager_t* comp_manager;

    bool is_dirty;
} rpe_rend_manager_t;

rpe_renderable_t rpe_renderable_init();

rpe_rend_manager_t* rpe_rend_manager_init(rpe_engine_t* engine, arena_t* arena);

rpe_renderable_t* rpe_rend_manager_get_mesh(rpe_rend_manager_t* m, rpe_object_t* obj);

arena_dyn_array_t*
rpe_rend_manager_batch_renderables(rpe_rend_manager_t* m, arena_dyn_array_t* object_arr);

#endif
