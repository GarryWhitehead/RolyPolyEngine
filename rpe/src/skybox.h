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

#ifndef __RPE_SKYBOX_H__
#define __RPE_SKYBOX_H__

#include "material.h"
#include "rpe/object.h"

#include <utility/maths.h>
#include <vulkan-api/resource_cache.h>

typedef struct Skybox
{
    texture_handle_t cube_texture;

    rpe_material_t material;
    rpe_object_t obj;
    rpe_mesh_t* cube_mesh;

    // not used if cube texture is specified
    math_vec4f bg_colour;

    // NOT USED
    bool show_sun;
} rpe_skybox_t;

rpe_skybox_t* rpe_skybox_init(rpe_engine_t* engine, arena_t* arena);

#endif
