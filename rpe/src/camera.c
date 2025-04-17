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

#include "camera.h"

#include "engine.h"
#include "frustum.h"

#include <string.h>

rpe_camera_t* rpe_camera_init(
    rpe_engine_t* engine,
    float fovy,
    uint32_t width,
    uint32_t height,
    float n,
    float f,
    enum ProjectionType type)
{
    rpe_camera_t* cam = ARENA_MAKE_ZERO_STRUCT(&engine->perm_arena, rpe_camera_t);
    rpe_camera_set_proj_matrix(cam, fovy, width, height, n, f, type);
    cam->view = math_mat4f_identity();
    cam->model = math_mat4f_identity();
    return cam;
}

math_vec3f rpe_camera_get_position(rpe_camera_t* cam)
{
    assert(cam);
    return math_vec3f_mul_sca(math_mat4f_translation_vec(cam->view), -1.0f);
}

void rpe_camera_set_proj_matrix(
    rpe_camera_t* cam,
    float fovy,
    uint32_t width,
    uint32_t height,
    float n,
    float f,
    enum ProjectionType type)
{
    assert(cam);
    assert(width > 0);
    assert(height > 0);
    assert(n <= f);

    cam->aspect = (float)width / (float)height;
    cam->projection = type == RPE_PROJECTION_TYPE_PERSPECTIVE
        ? math_mat4f_perspective(fovy, cam->aspect, n, f)
        : math_mat4f_ortho(0.0f, (float)width, 0.0f, (float)height, n, f);

    cam->fov = fovy;
    cam->n = n;
    cam->z = f;
    cam->type = type;
    cam->width = width;
    cam->height = height;
}

rpe_camera_ubo_t rpe_camera_update_ubo(rpe_camera_t* cam, rpe_frustum_t* f)
{
    assert(cam);
    math_mat4f mvp = math_mat4f_mul(cam->projection, math_mat4f_mul(cam->view, cam->model));
    rpe_camera_ubo_t ubo = {
        .mvp = mvp,
        .projection = cam->projection,
        .view = cam->view,
        .model = cam->model,
        .position = math_vec4f_init_vec3(rpe_camera_get_position(cam), 1.0f)};
    if (f)
    {
        memcpy(ubo.frustums, f->planes, sizeof(math_vec4f) * 6);
    }
    return ubo;
}

void rpe_camera_update_projection(rpe_camera_t* cam)
{
    rpe_camera_set_proj_matrix(cam, cam->fov, cam->width, cam->height, cam->n, cam->z, cam->type);
}

/** Public functions **/

void rpe_camera_set_projection(
    rpe_camera_t* cam,
    float fovy,
    uint32_t width,
    uint32_t height,
    float near,
    float far,
    enum ProjectionType type)
{
    assert(cam);
    rpe_camera_set_proj_matrix(cam, fovy, width, height, near, far, type);
}

void rpe_camera_set_view_matrix(rpe_camera_t* cam, math_mat4f* look_at)
{
    assert(cam);
    cam->view = *look_at;
}

void rpe_camera_set_fov(rpe_camera_t* cam, float fovy)
{
    assert(cam);
    cam->fov = fovy;
    rpe_camera_update_projection(cam);
}
