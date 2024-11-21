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
#ifndef __RPE_FRUSTUM_H__
#define __RPE_FRUSTUM_H__

#include <stdint.h>
#include <utility/maths.h>

// forward declarations
typedef struct AABBox rpe_aabox_t;

typedef struct Frustum
{
    math_vec4f planes[6];
} rpe_frustum_t;

void rpe_frustum_projection(rpe_frustum_t* f, math_mat4f* view_proj);

void rpe_frustum_check_intersection(
    rpe_frustum_t* f,
    math_vec3f* __restrict centers,
    math_vec3f* __restrict extents,
    size_t count,
    uint8_t* __restrict results);

bool rpe_frustum_check_intersection_aabox(rpe_frustum_t* f, rpe_aabox_t* box);

bool rpe_frustum_check_sphere_intersect(rpe_frustum_t* f, math_vec3f* center, float radius);

#endif