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

#ifndef __RPE_PRIV_CAMERA_H__
#define __RPE_PRIV_CAMERA_H__

#include "rpe/camera.h"

#include <utility/compiler.h>
#include <utility/maths.h>
#include <vulkan-api/resource_cache.h>

#ifdef WIN32
#undef near
#undef far
#endif

typedef struct Frustum rpe_frustum_t;
typedef struct Engine rpe_engine_t;

typedef struct CameraUbo
{
    math_mat4f mvp;
    math_mat4f projection;
    math_mat4f view;
    math_mat4f model;
    math_vec4f frustums[6];
    math_vec4f position;
} rpe_camera_ubo_t;

typedef struct Camera
{
    // current matrices
    math_mat4f projection;
    math_mat4f view;
    math_mat4f model;

    float fov;
    float n;
    float z;
    float aspect;
    uint32_t width;
    uint32_t height;
    enum ProjectionType type;

} rpe_camera_t;

void rpe_camera_set_proj_matrix(
    rpe_camera_t* cam,
    float fovy,
    uint32_t width,
    uint32_t height,
    float n,
    float f,
    enum ProjectionType type);

rpe_camera_ubo_t rpe_camera_update_ubo(rpe_camera_t* cam, rpe_frustum_t* f);

math_vec3f rpe_camera_get_position(rpe_camera_t* cam);

#endif
