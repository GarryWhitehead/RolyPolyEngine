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

#ifndef __GLTF_MATERIAL_CACHE_H__
#define __GLTF_MATERIAL_CACHE_H__

#include <rpe/material.h>
#include <vulkan-api/resource_cache.h>
#include <cgltf.h>

#define RPE_GLTF_IMAGE_CACHE_MAX_IMAGE_COUNT 500

typedef struct GltfImageHandle
{
    uint32_t id;
} gltf_image_handle_t;

enum DecodeStatus
{
    GLTF_IMAGE_CACHE_STATUS_PENDING,    ///< Waiting for decoding
    GLTF_IMAGE_CACHE_STATUS_NO_SCHEDULE        ///< No event scheduled
};

typedef struct ImageCache
{
    struct ImageEntry
    {
        rpe_mapped_texture_t texture;
        enum DecodeStatus status;
        cgltf_sampler* sampler;
        // This is set outside of the cache - following texture upload to the device.
        texture_handle_t backend_handle;

    } entries[RPE_GLTF_IMAGE_CACHE_MAX_IMAGE_COUNT];

    cgltf_data* root;
    uint32_t count;

} gltf_material_cache_t;

gltf_material_cache_t gltf_material_cache_init(cgltf_data* data);

gltf_image_handle_t gltf_material_cache_get_entry(gltf_material_cache_t* cache, cgltf_texture* texture);

gltf_image_handle_t gltf_material_cache_push_pending(gltf_material_cache_t* cache, cgltf_texture* texture);

struct ImageEntry* gltf_material_cache_get(gltf_material_cache_t* cache, gltf_image_handle_t handle);

#endif