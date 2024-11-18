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

#include "frustum.h"

#include "aabox.h"

void rpe_frustum_projection(rpe_frustum_t* f, math_mat4f* view_proj)
{
    assert(f);
    enum Face
    {
        Left,
        Right,
        Top,
        Bottom,
        Back,
        Front
    };

    f->planes[Left] = math_vec4f_sub(&view_proj->cols[3], &view_proj->cols[0]);
    f->planes[Right] = math_vec4f_add(&view_proj->cols[3], &view_proj->cols[0]);
    f->planes[Top] = math_vec4f_add(&view_proj->cols[3], &view_proj->cols[1]);
    f->planes[Bottom] = math_vec4f_sub(&view_proj->cols[3], &view_proj->cols[1]);
    f->planes[Front] = math_vec4f_sub(&view_proj->cols[3], &view_proj->cols[2]);
    f->planes[Back] = math_vec4f_add(&view_proj->cols[3], &view_proj->cols[2]);

    for (uint8_t i = 0; i < 6; ++i)
    {
        float len = math_vec4f_len(&f->planes[i]);
        math_vec4f_div_sca(&f->planes[i], len);
    }
}

void rpe_frustum_check_intersection(
    rpe_frustum_t* f,
    math_vec3f* __restrict centers,
    math_vec3f* __restrict extents,
    size_t count,
    uint8_t* __restrict results)
{
    assert(f);
    for (size_t i = 0; i < count; ++i)
    {
        bool visible = true;

#pragma unroll
        for (size_t j = 0; j < 6; ++j)
        {
            const float dot = f->planes[j].x * centers[i].x - fabsf(f->planes[j].x) * extents[i].x +
                f->planes[j].y * centers[i].y - fabsf(f->planes[j].y) * extents[i].y +
                f->planes[j].z * centers[i].z - fabsf(f->planes[j].z) * extents[i].z + f->planes[j].w;

            visible &= dot <= 0.0f;
        }
        results[i] = (uint8_t)visible;
    }
}

bool rpe_frustum_check_intersection_aabox(rpe_frustum_t* f, rpe_aabox_t* box)
{
    assert(f);
    uint8_t result = 0;
    math_vec3f center = rpe_aabox_get_center(box);
    math_vec3f extent = rpe_aabox_get_half_extent(box);
    rpe_frustum_check_intersection(f, &center, &extent, 1, &result);
    return (bool)result;
}

bool rpe_frustum_check_sphere_intersect(rpe_frustum_t* f, math_vec3f* center, float radius)
{
    assert(f);
    for (size_t i = 0; i < 6; ++i)
    {
        const float dot = f->planes[i].x * center->x + f->planes[i].y * center->y * f->planes[i].z +
            center->z + f->planes[i].w;
        if (dot <= -radius)
        {
            return false;
        }
    }
    return true;
}
