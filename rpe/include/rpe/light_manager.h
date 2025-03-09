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
#ifndef __RPE_LIGHT_MANAGER_H__
#define __RPE_LIGHT_MANAGER_H__

#include "object.h"

#include <utility/maths.h>

typedef struct LightManager rpe_light_manager_t;

enum LightType
{
    RPE_LIGHTING_TYPE_SPOT,
    RPE_LIGHTING_TYPE_POINT,
    RPE_LIGHTING_TYPE_DIRECTIONAL
};

typedef struct LightCreateInfo
{
    // position of the light in world space
    math_vec3f position;
    math_vec3f target;

    /// the colour of the light
    math_vec3f colour;

    /// the field of view of this light
    float fov;

    /// the light intensity in lumens
    float intensity;

    float fallout;
    float radius;
    float scale;
    float offset;

    // used for deriving the spotlight intensity
    float inner_cone;
    float outer_cone;

    // sun parameters (only used for directional light)
    float sun_angular_radius;
    float sun_halo_size;
    float sun_halo_falloff;
} rpe_light_create_info_t;

void rpe_light_manager_create_light(
    rpe_light_manager_t* lm, rpe_light_create_info_t* ci, rpe_object_t obj, enum LightType type);

#endif