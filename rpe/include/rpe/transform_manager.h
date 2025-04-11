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
#ifndef __RPE_TRANSFORM_MANAGER_H__
#define __RPE_TRANSFORM_MANAGER_H__

#include "object.h"

#include <utility/maths.h>

typedef struct TransformManager rpe_transform_manager_t;

typedef struct ModelTransform
{
    math_mat3f rot;
    math_vec3f scale;
    math_vec3f translation;
} rpe_model_transform_t;

rpe_model_transform_t rpe_model_transform_init();

void rpe_transform_manager_add_local_transform(
    rpe_transform_manager_t* m, rpe_model_transform_t* local_transform, rpe_object_t* obj);

void rpe_transform_manager_add_node(
    rpe_transform_manager_t* m,
    math_mat4f* local_transform,
    rpe_object_t* parent_obj,
    rpe_object_t* child_obj);

rpe_object_t* rpe_transform_manager_get_parent(rpe_transform_manager_t* m, rpe_object_t obj);
rpe_object_t* rpe_transform_manager_get_child(rpe_transform_manager_t* m, rpe_object_t obj);

void rpe_transform_manager_update_world(rpe_transform_manager_t* m, rpe_object_t obj);

void rpe_transform_manager_set_translation(
    rpe_transform_manager_t* m, rpe_object_t obj, math_vec3f trans);

void rpe_transform_manager_insert_node(
    rpe_transform_manager_t* m, rpe_object_t* new_obj, rpe_object_t* parent_obj);

rpe_object_t rpe_transform_manager_copy(
    rpe_transform_manager_t* tm,
    rpe_obj_manager_t* om,
    rpe_object_t* parent_obj,
    arena_dyn_array_t* objects);

#endif