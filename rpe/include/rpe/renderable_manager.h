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

rpe_mesh_t* rpe_rend_manager_create_mesh(
    rpe_rend_manager_t* m,
    float* pos_data,
    float* uv0_data,
    float* uv1_data,
    float* normal_data,
    float* tangent_data,
    float* col_data,
    float* bone_weight_data,
    float* bone_id_data,
    uint32_t vertex_size,
    void* indices,
    uint32_t indices_size,
    enum IndicesType indices_type);

// Convenience methods that make creating meshes easier.
rpe_mesh_t* rpe_rend_manager_create_static_mesh(
    rpe_rend_manager_t* m,
    float* pos_data,
    float* uv0_data,
    float* normal_data,
    float* col_data,
    uint32_t vertex_size,
    void* indices,
    uint32_t indices_size,
    enum IndicesType indices_type);

rpe_material_t* rpe_rend_manager_create_material(rpe_rend_manager_t* m);

void rpe_rend_manager_add(
    rpe_rend_manager_t* m,
    rpe_renderable_t* renderable,
    rpe_object_t rend_obj,
    rpe_object_t transform_obj);

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

void rpe_renderable_set_box(rpe_renderable_t* r, rpe_aabox_t* box);
void rpe_renderable_set_min_max_dimensions(rpe_renderable_t* r, math_vec3f min, math_vec3f max);

bool rpe_rend_manager_has_obj(rpe_rend_manager_t* m, rpe_object_t* obj);

rpe_object_t rpe_rend_manager_get_transform(rpe_rend_manager_t* rm, rpe_object_t rend_obj);

#endif