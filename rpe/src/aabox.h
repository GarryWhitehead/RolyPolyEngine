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

#ifndef __RPE_AABOX_H__
#define __RPE_AABOX_H__

#include "utility/maths.h"

typedef struct AABBox
{
    /// 3D extents (min, max) of this box in object space
    math_vec3f min;
    math_vec3f max;
} rpe_aabox_t;

static inline rpe_aabox_t rpe_aabox_init()
{
    rpe_aabox_t b = {
        .min.x = -1.0f,
        .min.y = -1.0f,
        .min.z = -1.0f,
        .max.x = 1.0f,
        .max.y = 1.0f,
        .max.z = 1.0f};
    return b;
}

static inline rpe_aabox_t rpe_aabox_calc_rigid_transform(rpe_aabox_t* box, math_mat4f* world)
{
    rpe_aabox_t out = rpe_aabox_init();
    math_mat3f rot = math_mat4f_to_rotation_matrix(world);
    math_vec3f t = math_mat4f_translation_vec(world);
    out.min = math_mat3f_mul_vec(&rot, &box->min);
    out.min = math_vec3f_add(&out.min, &t);
    out.max = math_mat3f_mul_vec(&rot, &box->max);
    out.max = math_vec3f_add(&out.max, &t);
    return out;
}

/**
 Calculates the center position of the box
 */
static inline math_vec3f rpe_aabox_get_center(rpe_aabox_t* b)
{
    math_vec3f c = math_vec3f_add(&b->max, &b->min);
    return (math_vec3f_mul_sca(&c, 0.5f));
}

/**
 Calculates the half extent of the box
 */
static inline math_vec3f rpe_aabox_get_half_extent(rpe_aabox_t* b)
{
    math_vec3f c = math_vec3f_sub(&b->max, &b->min);
    return (math_vec3f_mul_sca(&c, 0.5f));
}

#endif