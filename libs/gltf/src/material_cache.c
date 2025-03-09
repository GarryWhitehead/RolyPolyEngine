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

#include "material_cache.h"

gltf_material_cache_t gltf_material_cache_init(cgltf_data* data)
{
    assert(data);
    gltf_material_cache_t cache = {0};
    cache.root = data;
    for (int i = 0; i < RPE_GLTF_IMAGE_CACHE_MAX_IMAGE_COUNT; ++i)
    {
        cache.entries[i].status = GLTF_IMAGE_CACHE_STATUS_NO_SCHEDULE;
    }
    return cache;
}

gltf_image_handle_t gltf_material_cache_get_entry(gltf_material_cache_t* cache, cgltf_texture* texture)
{
    assert(cache);
    assert(texture);
    assert(texture->image);
    gltf_image_handle_t h = {.id = UINT32_MAX};

    long idx = texture->image - cache->root->images;
    assert(idx < RPE_GLTF_IMAGE_CACHE_MAX_IMAGE_COUNT);
    struct ImageEntry* entry = &cache->entries[idx];
    if (entry->status == GLTF_IMAGE_CACHE_STATUS_NO_SCHEDULE)
    {
        // No scheduled decoding event for this entry.
        return h;
    }
    h.id = idx;
    return h;
}

gltf_image_handle_t gltf_material_cache_push_pending(gltf_material_cache_t* cache, cgltf_texture* texture)
{
    assert(cache);
    assert(texture);

    long idx = texture->image - cache->root->images;
    assert(idx < RPE_GLTF_IMAGE_CACHE_MAX_IMAGE_COUNT);

    struct ImageEntry* entry = &cache->entries[idx];
    assert(entry->status == GLTF_IMAGE_CACHE_STATUS_NO_SCHEDULE);
    entry->status = GLTF_IMAGE_CACHE_STATUS_PENDING;
    entry->sampler = texture->sampler;
    ++cache->count;
    gltf_image_handle_t h = {.id = idx};
    return h;
}

struct ImageEntry* gltf_material_cache_get(gltf_material_cache_t* cache, gltf_image_handle_t handle)
{
    assert(cache);
    assert(handle.id != UINT32_MAX);
    return &cache->entries[handle.id];
}