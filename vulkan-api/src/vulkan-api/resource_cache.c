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

#include "resource_cache.h"

#include "commands.h"
#include "driver.h"
#include "texture.h"


bool vkapi_tex_handle_is_valid(texture_handle_t handle)
{
    return handle.id != VKAPI_INVALID_TEXTURE_HANDLE;
}
void vkapi_invalidate_tex_handle(texture_handle_t* handle)
{
    handle->id = VKAPI_INVALID_TEXTURE_HANDLE;
}

bool vkapi_buffer_handle_is_valid(buffer_handle_t handle)
{
    return handle.id != VKAPI_INVALID_BUFFER_HANDLE;
}
void vkapi_invalidate_buffer_handle(buffer_handle_t* handle)
{
    handle->id = VKAPI_INVALID_BUFFER_HANDLE;
}

vkapi_res_cache_t* vkapi_res_cache_init(vkapi_driver_t* driver, arena_t* arena)
{
    vkapi_res_cache_t* i = ARENA_MAKE_STRUCT(arena, vkapi_res_cache_t, ARENA_ZERO_MEMORY);
    MAKE_DYN_ARRAY(vkapi_texture_t, arena, 100, &i->textures);
    MAKE_DYN_ARRAY(vkapi_buffer_t, arena, 100, &i->buffers);
    MAKE_DYN_ARRAY(vkapi_texture_t, arena, 100, &i->textures_gc);
    MAKE_DYN_ARRAY(texture_handle_t, arena, 100, &i->free_tex_slots);
    MAKE_DYN_ARRAY(buffer_handle_t, arena, 100, &i->free_buffer_slots);

    i->vertex_data = ARENA_MAKE_ARRAY(arena, uint8_t, VKAPI_RES_CACHE_MAX_VERTEX_BUFFER_SIZE, 0);
    i->index_data = ARENA_MAKE_ARRAY(arena, uint8_t, VKAPI_RES_CACHE_MAX_INDEX_BUFFER_SIZE, 0);
    i->vertex_buffer = vkapi_buffer_init();
    i->index_buffer = vkapi_buffer_init();

    vkapi_buffer_alloc(
        &i->vertex_buffer,
        driver->vma_allocator,
        VKAPI_RES_CACHE_VERTEX_GPU_BUFFER_SIZE,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    vkapi_buffer_alloc(
        &i->index_buffer,
        driver->vma_allocator,
        VKAPI_RES_CACHE_INDEX_GPU_BUFFER_SIZE,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    return i;
}

texture_handle_t vkapi_res_cache_create_tex2d(
    vkapi_res_cache_t* cache,
    vkapi_context_t* context,
    VkFormat format,
    uint32_t width,
    uint32_t height,
    uint8_t mip_levels,
    uint8_t face_count,
    uint8_t array_count,
    VkImageUsageFlags usage_flags)
{
    assert(cache);
    vkapi_texture_t t =
        vkapi_texture_init(width, height, mip_levels, face_count, array_count, format);
    texture_handle_t handle = {.id = cache->textures.size};
    if (cache->free_tex_slots.size > 0)
    {
        handle = DYN_ARRAY_POP_BACK(texture_handle_t, &cache->free_tex_slots);
    }
    vkapi_texture_create_2d(context, &t, usage_flags);
    DYN_ARRAY_APPEND(&cache->textures, &t);
    return handle;
}

texture_handle_t vkapi_res_cache_push_tex2d(
    vkapi_res_cache_t* cache,
    vkapi_context_t* context,
    VkImage image,
    VkFormat format,
    uint32_t width,
    uint32_t height,
    uint8_t mip_levels,
    uint8_t face_count,
    uint8_t array_count)
{
    assert(cache);
    vkapi_texture_t t =
        vkapi_texture_init(width, height, mip_levels, face_count, array_count, format);
    t.image = image;
    t.image_views[0] = vkapi_texture_create_image_view(context, &t);
    texture_handle_t handle = {.id = cache->textures.size};
    if (cache->free_tex_slots.size > 0)
    {
        handle = DYN_ARRAY_POP_BACK(texture_handle_t, &cache->free_tex_slots);
    }
    DYN_ARRAY_APPEND(&cache->textures, &t);
    return handle;
}

vkapi_texture_t* vkapi_res_cache_get_tex2d(vkapi_res_cache_t* cache, texture_handle_t handle)
{
    assert(cache);
    assert(vkapi_tex_handle_is_valid(handle));
    assert(handle.id < cache->textures.size);
    return DYN_ARRAY_GET_PTR(vkapi_texture_t, &cache->textures, handle.id);
}

buffer_handle_t vkapi_res_cache_create_ubo(
    vkapi_res_cache_t* cache,
    vkapi_driver_t* driver,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    arena_t* arena)
{
    assert(cache);
    vkapi_buffer_t buffer = vkapi_buffer_init();
    buffer_handle_t handle = {.id = cache->buffers.size};
    if (cache->free_buffer_slots.size > 0)
    {
        handle = DYN_ARRAY_POP_BACK(buffer_handle_t, &cache->free_buffer_slots);
    }
    vkapi_buffer_alloc(&buffer, driver->vma_allocator, size, usage);
    DYN_ARRAY_APPEND(&cache->buffers, &buffer);
    return handle;
}

vkapi_buffer_t* vkapi_res_cache_get_buffer(vkapi_res_cache_t* cache, buffer_handle_t handle)
{
    assert(cache);
    assert(handle.id < cache->buffers.size);
    return DYN_ARRAY_GET_PTR(vkapi_buffer_t, &cache->buffers, handle.id);
}

void vkapi_res_cache_delete_tex2d(vkapi_res_cache_t* cache, texture_handle_t handle)
{
    assert(cache);
    assert(handle.id < cache->textures.size);

    // May have already been deleted - should this be an error?
    if (!vkapi_tex_handle_is_valid(handle))
    {
        return;
    }

    vkapi_texture_t t = DYN_ARRAY_GET(vkapi_texture_t, &cache->textures, handle.id);
    t.frames_until_gc = VKAPI_MAX_COMMAND_BUFFER_SIZE;
    DYN_ARRAY_APPEND(&cache->textures_gc, &t);
    DYN_ARRAY_APPEND(&cache->free_tex_slots, &handle);
}

uint32_t vkapi_res_cache_create_vert_buffer(
    vkapi_res_cache_t* cache, vkapi_driver_t* driver, size_t size, void* data)
{
    assert(cache);
    assert(data);
    assert(cache->vertex_offset + size < VKAPI_RES_CACHE_MAX_VERTEX_BUFFER_SIZE);
    uint8_t* ptr = cache->vertex_data + cache->vertex_offset;
    memcpy(ptr, data, size);
    uint32_t out = cache->vertex_offset;
    cache->vertex_offset += size;
    return out;
}

uint32_t vkapi_res_cache_create_index_buffer(
    vkapi_res_cache_t* cache, vkapi_driver_t* driver, size_t size, void* data)
{
    assert(cache);
    assert(data);
    assert(cache->index_offset + size < VKAPI_RES_CACHE_MAX_INDEX_BUFFER_SIZE);
    uint8_t* ptr = cache->index_data + cache->index_offset;
    memcpy(ptr, data, size);
    uint32_t out = cache->index_offset;
    cache->index_offset += size;
    return out;
}

void vkapi_res_cache_upload_vertex_buffer(vkapi_res_cache_t* cache)
{
    assert(cache);
    vkapi_buffer_map_to_gpu_buffer(
        &cache->vertex_buffer, cache->vertex_data, cache->vertex_offset, 0);
}

void vkapi_res_cache_upload_index_buffer(vkapi_res_cache_t* cache)
{
    assert(cache);
    vkapi_buffer_map_to_gpu_buffer(&cache->index_buffer, cache->index_data, cache->index_offset, 0);
}

void vkapi_res_cache_gc(vkapi_res_cache_t* c, vkapi_driver_t* driver)
{
    assert(c);
    arena_dyn_array_t new_gc;
    for (size_t i = 0; i < c->textures_gc.size; ++i)
    {
        vkapi_texture_t* tex = DYN_ARRAY_GET_PTR(vkapi_texture_t, &c->textures_gc, i);
        if (!--tex->frames_until_gc)
        {
            vkapi_texture_destroy(driver->context, tex);
        }
        else
        {
            DYN_ARRAY_APPEND(&new_gc, tex);
        }
    }
    c->textures_gc = new_gc;

    arena_dyn_array_t new_buffer_gc;
    for (size_t i = 0; i < c->buffers_gc.size; ++i)
    {
        vkapi_buffer_t* b = DYN_ARRAY_GET_PTR(vkapi_buffer_t, &c->buffers_gc, i);
        if (!--b->frames_until_gc)
        {
            vkapi_buffer_destroy(b, driver->vma_allocator);
        }
        else
        {
            DYN_ARRAY_APPEND(&new_buffer_gc, b);
        }
    }
    c->buffers_gc = new_buffer_gc;
}

void vkapi_res_cache_destroy(vkapi_res_cache_t* c, vkapi_driver_t* driver)
{
    for (size_t i = 0; i < c->textures.size; ++i)
    {
        vkapi_texture_t* t = DYN_ARRAY_GET_PTR(vkapi_texture_t, &c->textures, i);
        vkapi_texture_destroy(driver->context, t);
    }
    dyn_array_clear(&c->textures);

    for (size_t i = 0; i < c->buffers.size; ++i)
    {
        vkapi_buffer_t* b = DYN_ARRAY_GET_PTR(vkapi_buffer_t, &c->buffers, i);
        vkapi_buffer_destroy(b, driver->vma_allocator);
    }
    dyn_array_clear(&c->buffers);

    vkapi_buffer_destroy(&c->vertex_buffer, driver->vma_allocator);
    vkapi_buffer_destroy(&c->index_buffer, driver->vma_allocator);
}
