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

#ifndef __APP_CAMERA_H__
#define __APP_CAMERA_H__

#include <utility/maths.h>

typedef struct Camera rpe_camera_t;

enum ProjectionType
{
    RPE_PROJECTION_TYPE_PERSPECTIVE,
    RPE_PROJECTION_TYPE_ORTHO
};

void rpe_camera_set_projection(
    rpe_camera_t* cam, float fovy, float aspect, float near, float far, enum ProjectionType type);

void rpe_camera_set_view_matrix(rpe_camera_t* cam, math_mat4f* look_at);

void rpe_camera_set_fov(rpe_camera_t* cam, float fovy);

#endif