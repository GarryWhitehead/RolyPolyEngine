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
    if (type == RPE_CAMERA_TYPE_PERSPECTIVE)
    {
        cam->projection = math_mat4f_projection(fovy, aspect, n, f);
    }
    else
    {
        // TODO: add ortho
    }

    cam->aspect = aspect;
    cam->fov = fovy;
    cam->n= n;
    cam->z = f;
}
