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

#include "camera_view.h"

#include "engine.h"
#include "window.h"

#include <utility/hash.h>

rpe_camera_view_t rpe_camera_view_init(rpe_engine_t* engine)
{
    rpe_camera_view_t cam_view = {0};
    cam_view.move_speed = 0.2f;
    cam_view.key_events = HASH_SET_CREATE(enum MovementType, bool, &engine->perm_arena);
    cam_view.view = math_mat4f_identity();
    cam_view.eye.z = -2.0f;
    return cam_view;
}

void rpe_camera_view_key_up_event(rpe_camera_view_t* cam_view, enum MovementType movement)
{
    assert(cam_view);
    bool state = false;
    HASH_SET_SET(&cam_view->key_events, &movement, &state);
}

void rpe_camera_view_key_down_event(rpe_camera_view_t* cam_view, enum MovementType movement)
{
    assert(cam_view);
    bool state = true;
    HASH_SET_SET(&cam_view->key_events, &movement, &state);
}

void rpe_camera_view_mouse_button_down(rpe_camera_view_t* cam_view, double x, double y)
{
    assert(cam_view);
    cam_view->mouse_position.x = (float)x;
    cam_view->mouse_position.y = (float)y;
    cam_view->mouse_button_down = true;
}

void rpe_camera_view_mouse_update(rpe_camera_view_t* cam_view, double x, double y)
{
    if (!cam_view->mouse_button_down)
    {
        return;
    }

    float dx = x - cam_view->mouse_position.x;
    float dy = cam_view->mouse_position.y - y;
    cam_view->mouse_position.x = x;
    cam_view->mouse_position.y = y;

    const float min_pitch = (float)(-M_PI_2 + 0.001);
    const float max_pitch = (float)(M_PI_2 - 0.001);
    float pitch = CLAMP(dy * cam_view->move_speed, min_pitch, max_pitch);
    float yaw = dx * cam_view->move_speed;

    cam_view->rotation.y += pitch;
    cam_view->rotation.x += yaw;

    rpe_camera_view_update_view(cam_view);
}

void rpe_camera_view_mouse_button_up(rpe_camera_view_t* cam_view)
{
    assert(cam_view);
    cam_view->mouse_button_down = false;
}

math_vec3f rpe_camera_view_front_vec(rpe_camera_view_t* cam_view)
{
    assert(cam_view);
    math_vec3f out = {
        -cosf(math_to_radians(cam_view->rotation.y)) * sinf(math_to_radians(cam_view->rotation.x)),
        sinf(math_to_radians(cam_view->rotation.y)),
        cosf(math_to_radians(cam_view->rotation.y)) * cosf(math_to_radians(cam_view->rotation.x))};
    return out;
}

void rpe_camera_view_update_view(rpe_camera_view_t* cam_view)
{
    assert(cam_view);
    math_vec3f up = {0.0f, 1.0f, 0.0f};
    math_vec3f front = rpe_camera_view_front_vec(cam_view);
    cam_view->view = math_mat4f_lookat(math_vec3f_add(cam_view->eye, front), cam_view->eye, up);
}

math_vec3f rpe_camera_view_right_vec(math_vec3f front)
{
    math_vec3f up = {0.0f, 1.0f, 0.0f};
    return math_vec3f_normalise(math_vec3f_cross(front, up));
}

bool get_movement_state(rpe_camera_view_t* cam_view, enum MovementType* type)
{
    bool* ptr = HASH_SET_GET(&cam_view->key_events, type);
    if (!ptr)
    {
        return false;
    }
    return *ptr;
}

void rpe_camera_view_update_key_events(rpe_camera_view_t* cam_view, float dt)
{
    float speed = cam_view->move_speed * dt;

    math_vec3f front = rpe_camera_view_front_vec(cam_view);
    math_vec3f right = rpe_camera_view_right_vec(front);

    enum MovementType type = RPE_MOVEMENT_FORWARD;
    if (get_movement_state(cam_view, &type))
    {
        cam_view->eye = math_vec3f_add(cam_view->eye, math_vec3f_mul_sca(front, speed));
    }
    type = RPE_MOVEMENT_BACKWARD;
    if (get_movement_state(cam_view, &type))
    {
        cam_view->eye = math_vec3f_sub(cam_view->eye, math_vec3f_mul_sca(front, speed));
    }
    type = RPE_MOVEMENT_LEFT;
    if (get_movement_state(cam_view, &type))
    {
        cam_view->eye = math_vec3f_sub(cam_view->eye, math_vec3f_mul_sca(right, speed));
    }
    type = RPE_MOVEMENT_RIGHT;
    if (get_movement_state(cam_view, &type))
    {
        cam_view->eye = math_vec3f_add(cam_view->eye, math_vec3f_mul_sca(right, speed));
    }

    rpe_camera_view_update_view(cam_view);
}

void rpe_camera_view_set_position(rpe_camera_view_t* cam_view, const math_vec3f pos)
{
    cam_view->eye = pos;
    rpe_camera_view_update_view(cam_view);
}

enum MovementType rpe_camera_view_convert_key_code(int code)
{
    enum MovementType out;
    switch (code)
    {
        case GLFW_KEY_W:
            out = RPE_MOVEMENT_FORWARD;
            break;
        case GLFW_KEY_S:
            out = RPE_MOVEMENT_BACKWARD;
            break;
        case GLFW_KEY_A:
            out = RPE_MOVEMENT_LEFT;
            break;
        case GLFW_KEY_D:
            out = RPE_MOVEMENT_RIGHT;
            break;
        default:
            out = RPE_MOVEMENT_NONE;
    }
    return out;
}
