/* Copyright (c) 2024-2025 Garry Whitehead
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

#ifndef __APP_CAMERA_VIEW_H__
#define __APP_CAMERA_VIEW_H__

#include <utility/hash_set.h>
#include <utility/maths.h>

typedef struct Engine rpe_engine_t;

enum MovementType
{
    RPE_MOVEMENT_FORWARD,
    RPE_MOVEMENT_BACKWARD,
    RPE_MOVEMENT_LEFT,
    RPE_MOVEMENT_RIGHT,
    RPE_MOVEMENT_NONE
};

typedef struct CameraView
{
    math_mat4f view;
    math_vec3f eye;
    math_vec3f rotation;

    hash_set_t key_events;

    math_vec2f mouse_position;
    bool mouse_button_down;

    float move_speed;
} rpe_camera_view_t;

rpe_camera_view_t rpe_camera_view_init(rpe_engine_t* engine);

void rpe_camera_view_key_up_event(rpe_camera_view_t* cam_view, enum MovementType movement);

void rpe_camera_view_key_down_event(rpe_camera_view_t* cam_view, enum MovementType movement);

void rpe_camera_view_mouse_button_down(rpe_camera_view_t* cam_view, double x, double y);

void rpe_camera_view_mouse_update(rpe_camera_view_t* cam_view, double x, double y);

void rpe_camera_view_mouse_button_up(rpe_camera_view_t* cam_view);

void rpe_camera_view_update_view(rpe_camera_view_t* cam_view);

void rpe_camera_view_update_key_events(rpe_camera_view_t* cam_view, float dt);

void rpe_camera_view_set_position(rpe_camera_view_t* cam_view, math_vec3f pos);

enum MovementType rpe_camera_view_convert_key_code(int code);

#endif