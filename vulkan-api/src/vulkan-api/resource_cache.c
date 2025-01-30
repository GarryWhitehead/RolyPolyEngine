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
#include "utility.h"


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
    MAKE_DYN_ARRAY(vkapi_buffer_t, arena, 20, &i->buffers);
    MAKE_DYN_ARRAY(vkapi_texture_t, arena, 50, &i->textures_gc);
    MAKE_DYN_ARRAY(texture_handle_t, arena, 20, &i->free_tex_slots);
    MAKE_DYN_ARRAY(buffer_handle_t, arena, 100, &i->free_buffer_slots);
    // Four slots are reserved for special swapchain textures.
    dyn_array_resize(&i->textures, VKAPI_RES_CACHE_MAX_RESERVED_COUNT);
    return i;
}

texture_handle_t vkapi_res_push_reserved_tex2d(
    vkapi_res_cache_t* cache,
    vkapi_context_t* context,
    uint32_t width,
    uint32_t height,
    VkFormat format,
    uint32_t idx,
    VkImageUsageFlags usage_flags,
    VkImage* image)
{
    assert(cache);
    assert(idx < VKAPI_RES_CACHE_MAX_RESERVED_COUNT);
    vkapi_texture_t tex = vkapi_texture_init(width, height, 1, 1, 1, format);

    if (image)
    {
        tex.image = *image;
    }
    else
    {
        vkapi_texture_create_image(context, &tex, usage_flags);
    }

    tex.image_views[0] = vkapi_texture_create_image_view(context, &tex);
    tex.image_layout = (vkapi_util_is_depth(format) || vkapi_util_is_stencil(format))
        ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL
        : (usage_flags & VK_IMAGE_USAGE_STORAGE_BIT) ? VK_IMAGE_LAYOUT_GENERAL
                                                     : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    DYN_ARRAY_SET(&cache->textures, idx, &tex);
    texture_handle_t handle = {.id = idx};
    return handle;
}

texture_handle_t vkapi_res_cache_create_tex2d(
    vkapi_res_cache_t* cache,
    vkapi_context_t* context,
    vkapi_sampler_cache_t* sampler_cache,
    VkFormat format,
    uint32_t width,
    uint32_t height,
    uint8_t mip_levels,
    uint8_t face_count,
    uint8_t array_count,
    VkImageUsageFlags usage_flags,
    sampler_params_t* sampler_params)
{
    vkapi_texture_t t =
        vkapi_texture_init(width, height, mip_levels, face_count, array_count, format);
    texture_handle_t handle = {.id = cache->textures.size};
    vkapi_texture_create_2d(context, sampler_cache, &t, usage_flags, sampler_params);
    if (cache->free_tex_slots.size > 0)
    {
        handle = DYN_ARRAY_POP_BACK(texture_handle_t, &cache->free_tex_slots);
        assert(handle.id >= VKAPI_RES_CACHE_MAX_RESERVED_COUNT);
        DYN_ARRAY_SET(&cache->textures, handle.id, &t);
    }
    else
    {
        DYN_ARRAY_APPEND(&cache->textures, &t);
    }
    return handle;
}

vkapi_texture_t* vkapi_res_cache_get_tex2d(vkapi_res_cache_t* cache, texture_handle_t handle)
{
    assert(cache);
    assert(vkapi_tex_handle_is_valid(handle));
    assert(handle.id < cache->textures.size);
    return DYN_ARRAY_GET_PTR(vkapi_texture_t, &cache->textures, handle.id);
}

buffer_handle_t vkapi_res_cache_create_buffer(
    vkapi_res_cache_t* cache,
    vkapi_driver_t* driver,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    enum BufferType type)
{
    assert(cache);
    vkapi_buffer_t buffer = vkapi_buffer_init();
    buffer_handle_t handle = {.id = cache->buffers.size};
    vkapi_buffer_alloc(&buffer, driver->vma_allocator, size, usage, type);
    if (cache->free_buffer_slots.size > 0)
    {
        handle = DYN_ARRAY_POP_BACK(buffer_handle_t, &cache->free_buffer_slots);
        DYN_ARRAY_SET(&cache->buffers, handle.id, &buffer);
    }
    else
    {
        DYN_ARRAY_APPEND(&cache->buffers, &buffer);
    }
    return handle;
}

buffer_handle_t
vkapi_res_cache_create_ubo(vkapi_res_cache_t* cache, vkapi_driver_t* driver, VkDeviceSize size)
{
    return vkapi_res_cache_create_buffer(
        cache, driver, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VKAPI_BUFFER_HOST_TO_GPU);
}

buffer_handle_t vkapi_res_cache_create_ssbo(
    vkapi_res_cache_t* cache,
    vkapi_driver_t* driver,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    enum BufferType type)
{
    return vkapi_res_cache_create_buffer(
        cache, driver, size, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | usage, type);
}

buffer_handle_t vkapi_res_cache_create_vertex_buffer(
    vkapi_res_cache_t* cache, vkapi_driver_t* driver, VkDeviceSize size)
{
    return vkapi_res_cache_create_buffer(
        cache, driver, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VKAPI_BUFFER_HOST_TO_GPU);
}

buffer_handle_t vkapi_res_cache_create_index_buffer(
    vkapi_res_cache_t* cache, vkapi_driver_t* driver, VkDeviceSize size)
{
    return vkapi_res_cache_create_buffer(
        cache, driver, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VKAPI_BUFFER_GPU_ONLY);
}

vkapi_buffer_t* vkapi_res_cache_get_buffer(vkapi_res_cache_t* cache, buffer_handle_t handle)
{
    assert(cache);
    assert(handle.id < cache->buffers.size);
    return DYN_ARRAY_GET_PTR(vkapi_buffer_t, &cache->buffers, handle.id);
}

void vkapi_res_cache_delete_tex2d(vkapi_res_cache_t* cache, texture_handle_t handle)
{
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

void vkapi_res_cache_gc(vkapi_res_cache_t* c, vkapi_driver_t* driver)
{
    assert(c);
    uint32_t curr_count = 0;
    for (size_t i = 0; i < c->textures_gc.size; ++i)
    {
        vkapi_texture_t* tex = DYN_ARRAY_GET_PTR(vkapi_texture_t, &c->textures_gc, i);
        if (!--tex->frames_until_gc)
        {
            vkapi_texture_destroy(driver->context, tex);
        }
        else
        {
            DYN_ARRAY_SET(&c->textures_gc, curr_count++, tex);
        }
    }
    dyn_array_shrink(&c->textures_gc, curr_count);

    curr_count = 0;
    for (size_t i = 0; i < c->buffers_gc.size; ++i)
    {
        vkapi_buffer_t* b = DYN_ARRAY_GET_PTR(vkapi_buffer_t, &c->buffers_gc, i);
        if (!--b->frames_until_gc)
        {
            vkapi_buffer_destroy(b, driver->vma_allocator);
        }
        else
        {
            DYN_ARRAY_SET(&c->buffers_gc, curr_count++, b);
        }
    }
    dyn_array_shrink(&c->buffers_gc, curr_count);
}

void vkapi_res_cache_destroy(vkapi_res_cache_t* c, vkapi_driver_t* driver)
{
    for (size_t i = VKAPI_RES_CACHE_MAX_RESERVED_COUNT; i < c->textures.size; ++i)
    {
        // Only destroy textures which have not been marked as "free" slots - these will be deleted
        // in the garbage collection set.
        if (!dyn_array_find(&c->free_tex_slots, &i))
        {
            vkapi_texture_t* t = DYN_ARRAY_GET_PTR(vkapi_texture_t, &c->textures, i);
            vkapi_texture_destroy(driver->context, t);
        }
    }
    for (size_t i = 0; i < c->textures_gc.size; ++i)
    {
        vkapi_texture_t* t = DYN_ARRAY_GET_PTR(vkapi_texture_t, &c->textures_gc, i);
        vkapi_texture_destroy(driver->context, t);
    }
    dyn_array_clear(&c->textures);
    dyn_array_clear(&c->textures_gc);

    for (size_t i = 0; i < c->buffers.size; ++i)
    {
        // Only destroy buffers which have not been marked as "free" slots - these will be deleted
        // in the garbage collection set.
        if (!dyn_array_find(&c->free_buffer_slots, &i))
        {
            vkapi_buffer_t* b = DYN_ARRAY_GET_PTR(vkapi_buffer_t, &c->buffers, i);
            vkapi_buffer_destroy(b, driver->vma_allocator);
        }
    }
    for (size_t i = 0; i < c->buffers_gc.size; ++i)
    {
        vkapi_buffer_t* b = DYN_ARRAY_GET_PTR(vkapi_buffer_t, &c->buffers_gc, i);
        vkapi_buffer_destroy(b, driver->vma_allocator);
    }
    dyn_array_clear(&c->buffers);
    dyn_array_clear(&c->buffers_gc);
}
