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

#include "frustum.h"

#include <backend/enums.h>
#include <vulkan-api/driver.h>

rpe_camera_t rpe_camera_init(vkapi_driver_t* driver)
{
    rpe_camera_t cam;
    memset(&cam, 0, sizeof(rpe_camera_t));
    cam.projection = math_mat4f_identity();
    cam.view = math_mat4f_identity();
    cam.model = math_mat4f_identity();
    return cam;
}

void rpe_camera_set_proj_matrix(
    rpe_camera_t* cam, float fovy, float aspect, float n, float f, enum ProjectionType type)
{
    assert(cam);
    if (type == RPE_PROJECTION_TYPE_PERSPECTIVE)
    {
        cam->projection = math_mat4f_projection(math_to_radians(fovy), aspect, n, f);
    }
    else
    {
        // TODO: add ortho
    }

    cam->aspect = aspect;
    cam->fov = fovy;
    cam->n = n;
    cam->z = f;
}

rpe_camera_ubo_t rpe_camera_update_ubo(rpe_camera_t* cam, rpe_frustum_t* f)
{
    assert(cam);
    assert(f);
    math_mat4f mvp = math_mat4f_mul(cam->projection, math_mat4f_mul(cam->view, cam->model));
    rpe_camera_ubo_t ubo = {
        .mvp = mvp, .projection = cam->projection, .view = cam->view, .model = cam->model};
    memcpy(ubo.frustums, f->planes, sizeof(math_vec4f) * 6);
    return ubo;
}

/** Public functions **/

void rpe_camera_set_projection(
    rpe_camera_t* cam, float fovy, float aspect, float near, float far, enum ProjectionType type)
{
    assert(cam);
    rpe_camera_set_proj_matrix(cam, fovy, aspect, near, far, type);
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
}
