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

#ifndef __GLTF_RESOURCE_LOADER_PRIV_H__
#define __GLTF_RESOURCE_LOADER_PRIV_H__

#include "gltf/gltf_asset.h"
#include "material_cache.h"

#include <utility/arena.h>
#include <utility/job_queue.h>
#include <utility/string.h>

typedef struct GltfAsset gltf_asset_t;

struct DecodeEntry
{
    void* image_data;
    size_t image_sz;
    rpe_mapped_texture_t* mapped_texture;
    image_free_func* free_func;
    string_t mime_type;
    job_t* decoder_job;
};

typedef struct ResoureLoader
{
    gltf_material_cache_t texture_cache;
    arena_dyn_array_t decode_queue;
    job_t* parent_job;
} gltf_resource_loader_t;

#endif