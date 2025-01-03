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
typedef struct Mesh rpe_mesh_t;
typedef struct Material rpe_material_t;
typedef struct Renderable rpe_renderable_t;


enum PrimitiveFlags
{
    RPE_RENDERABLE_PRIM_HAS_SKIN = 1 << 0,
    RPE_RENDERABLE_PRIM_HAS_JOINTS = 1 << 1
};

rpe_mesh_t* rpe_rend_manager_create_mesh(
    rpe_rend_manager_t* m,
    math_vec3f* pos_data,
    math_vec2f* uv_data,
    math_vec3f* normal_data,
    math_vec4f* col_data,
    math_vec4f* bone_weight_data,
    math_vec4f* bone_id_data,
    uint32_t vertex_size,
    int32_t* indices,
    uint32_t indices_size);

rpe_material_t* rpe_rend_manager_create_material(rpe_rend_manager_t* m);

void rpe_rend_manager_add(rpe_rend_manager_t* m, rpe_renderable_t* renderable, rpe_object_t obj);

#endif