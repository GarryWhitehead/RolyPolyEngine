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
#ifndef __RPE_SCENE_H__
#define __RPE_SCENE_H__

#include "object.h"

typedef struct Scene rpe_scene_t;
typedef struct Engine rpe_engine_t;
typedef struct Camera rpe_camera_t;
typedef struct Ibl ibl_t;
typedef struct Skybox rpe_skybox_t;

void rpe_scene_set_current_camera(rpe_scene_t* scene, rpe_engine_t* engine, rpe_camera_t* cam);

void rpe_scene_set_ibl(rpe_scene_t* scene, ibl_t* ibl);

void rpe_scene_add_object(rpe_scene_t* scene, rpe_object_t obj);

void rpe_scene_set_current_skyox(rpe_scene_t* scene, rpe_skybox_t* sb);

#endif