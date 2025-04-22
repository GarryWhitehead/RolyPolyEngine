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

#ifndef __GLTF_MODEL_H__
#define __GLTF_MODEL_H__

#include <utility/maths.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct GltfAsset gltf_asset_t;
typedef struct Engine rpe_engine_t;
typedef struct RenderableManager rpe_rend_manager_t;
typedef struct TransformManager rpe_transform_manager_t;
typedef struct ObjectManager rpe_obj_manager_t;
typedef struct Scene rpe_scene_t;
typedef struct Arena arena_t;

gltf_asset_t*
gltf_model_parse_data(uint8_t* gltf_data, size_t data_size, rpe_engine_t* engine, const char* path, arena_t* arena);

// Create a specified number of model instances - the model data must have been parsed, and the
// assets' data struct populated before calling this function.
void gltf_model_create_instances(
    gltf_asset_t* assets,
    rpe_rend_manager_t* rm,
    rpe_transform_manager_t* tm,
    rpe_obj_manager_t* om,
    rpe_scene_t* scene,
    uint32_t count,
    math_vec3f* translations,
    arena_t* arena);

#endif
