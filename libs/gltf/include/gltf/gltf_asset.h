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

#ifndef __GLTF_ASSET_H__
#define __GLTF_ASSET_H__

#include "material_cache.h"
#include <cgltf.h>
#include <rpe/aabox.h>
#include <rpe/material.h>
#include <utility/arena.h>
#include <utility/string.h>

typedef struct Engine rpe_engine_t;
typedef struct Material rpe_material_t;
typedef struct RenderableManager rpe_rend_manager_t;

#define GLTF_ASSET_ARENA_SIZE 1 << 30

typedef void (*image_free_func)(void*);

typedef struct AssetTextureParams
{
    rpe_material_t* mat;
    cgltf_texture_view* gltf_tex;
    // This is updated via the appropriate loader.
    gltf_image_handle_t mat_texture;
    enum MaterialImageType tex_type;
    uint32_t uv_index;
    image_free_func free_func;
} asset_texture_t;

typedef struct GltfAsset
{
    rpe_engine_t* engine;
    rpe_rend_manager_t* rend_manager;
    cgltf_data* model_data;
    arena_dyn_array_t textures;
    arena_dyn_array_t materials;
    arena_dyn_array_t meshes;
    arena_dyn_array_t objects;
    arena_dyn_array_t nodes;
    rpe_aabox_t aabbox;
    string_t gltf_path;
    arena_t arena;
} gltf_asset_t;

#endif