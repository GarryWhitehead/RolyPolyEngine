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
#ifndef __RPE_RENDERABLE_MANAGER_H__
#define __RPE_RENDERABLE_MANAGER_H__

#include "object.h"

#include <stdint.h>
#include <utility/maths.h>

typedef struct RenderableManager rpe_rend_manager_t;
typedef struct TransformManager rpe_transform_manager_t;
typedef struct Mesh rpe_mesh_t;
typedef struct Material rpe_material_t;
typedef struct Renderable rpe_renderable_t;
typedef struct AABBox rpe_aabox_t;
typedef struct Scene rpe_scene_t;

#define RPE_RENDERABLE_MAX_UV_SET_COUNT 2

enum PrimitiveFlags
{
    RPE_RENDERABLE_PRIM_HAS_SKIN = 1 << 0,
    RPE_RENDERABLE_PRIM_HAS_JOINTS = 1 << 1
};

enum IndicesType
{
    RPE_RENDERABLE_INDICES_U32,
    RPE_RENDERABLE_INDICES_U16
};

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

// Note: On linux and Windows we get an extra 8bytes of packing, so disable otherwise
// messes up attribute strides on the shader.
typedef struct Vertex
{
    float position[3];
    float normal[3];
    float uv0[2];
    float uv1[2];
    float tangent[4];
    float colour[4];
    float bone_weight[4];
    float bone_id[4];
} rpe_vertex_t;
static_assert(sizeof(struct Vertex) == 104, "Vertex struct must have no padding.");

typedef struct VertexAllocHandle
{
    uint32_t id;
} rpe_valloc_handle;

rpe_mesh_t* rpe_rend_manager_create_mesh_interleaved(
    rpe_rend_manager_t* m,
    rpe_valloc_handle v_handle,
    float* pos_data,
    float* uv0_data,
    float* uv1_data,
    float* normal_data,
    float* tangent_data,
    float* col_data,
    float* bone_weight_data,
    float* bone_id_data,
    uint32_t vertex_size,
    rpe_valloc_handle i_handle,
    void* indices,
    uint32_t indices_size,
    enum IndicesType indices_type);

/**
 Create a mesh and upload to the device.
 Note: This requires the vertex data to be interleaved in the format used internally which
 is found above in the `struct Vertex` data layout.
 @param m
 @param vertex_data
 @param vertex_size
 @param indices
 @param indices_size
 @param indices_type
 @param mesh_flags
 */
rpe_mesh_t* rpe_rend_manager_create_mesh(
    rpe_rend_manager_t* m,
    rpe_valloc_handle v_handle,
    rpe_vertex_t* vertex_data,
    uint32_t vertex_size,
    rpe_valloc_handle i_handle,
    void* indices,
    uint32_t indices_size,
    enum IndicesType indices_type,
    enum MeshAttributeFlags mesh_flags);

// Convenience methods that make creating meshes easier.
rpe_mesh_t* rpe_rend_manager_create_static_mesh(
    rpe_rend_manager_t* m,
    rpe_valloc_handle v_handle,
    float* pos_data,
    float* uv0_data,
    float* normal_data,
    float* col_data,
    uint32_t vertex_size,
    rpe_valloc_handle i_handle,
    void* indices,
    uint32_t indices_size,
    enum IndicesType indices_type);

/**
 Use when a mesh contains numerous primitives based upon the same set of vertices.
 @param m
 @param mesh
 @pram index_offset The offset which will be applied to the base index stated by the original @sa
 mesh.
 @param index_count The number of indices to draw.
 @returns a new mesh instance with the index_base + indices_offset.
 */
rpe_mesh_t* rpe_rend_manager_offset_indices(
    rpe_rend_manager_t* m, rpe_mesh_t* mesh, uint32_t index_offset, uint32_t index_count);

rpe_material_t* rpe_rend_manager_create_material(rpe_rend_manager_t* m, rpe_scene_t* scene);

void rpe_rend_manager_add(
    rpe_rend_manager_t* m,
    rpe_renderable_t* renderable,
    rpe_object_t rend_obj,
    rpe_object_t transform_obj);

bool rpe_rend_manager_remove(rpe_rend_manager_t* rm, rpe_object_t obj);

/**
 Copy a renderable object from src to dst. Optional override of transform associated with this
 renderable.
 */
void rpe_rend_manager_copy(
    rpe_rend_manager_t* rm,
    rpe_transform_manager_t* tm,
    rpe_object_t src_obj,
    rpe_object_t dst_obj,
    rpe_object_t* transform_obj);

rpe_valloc_handle rpe_rend_manager_alloc_vertex_buffer(rpe_rend_manager_t* m, uint32_t vertex_size);
rpe_valloc_handle rpe_rend_manager_alloc_index_buffer(rpe_rend_manager_t* m, uint32_t index_size);

bool rpe_rend_manager_has_obj(rpe_rend_manager_t* m, rpe_object_t* obj);

rpe_object_t rpe_rend_manager_get_transform(rpe_rend_manager_t* rm, rpe_object_t rend_obj);

/****** Renderable functions *************/

void rpe_renderable_set_box(rpe_renderable_t* r, rpe_aabox_t* box);

void rpe_renderable_set_min_max_dimensions(rpe_renderable_t* r, math_vec3f min, math_vec3f max);

void rpe_renderable_set_scissor(rpe_renderable_t* r, int32_t x, int32_t y, uint32_t w, uint32_t h);

void rpe_renderable_set_viewport(
    rpe_renderable_t* r,
    int32_t x,
    int32_t y,
    uint32_t w,
    uint32_t h,
    float min_depth,
    float max_depth);

void rpe_renderable_set_view_layer(rpe_renderable_t* r, uint8_t layer);

/// Frustum culling is enabled by default. This disables the check so the renderable will always be drawn.
void rpe_renderable_disbale_frustum_culling(rpe_renderable_t* r);

#endif