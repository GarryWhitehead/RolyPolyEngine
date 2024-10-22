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
#include "texture.h"

bool vkapi_tex_handle_is_valid(texture_handle_t handle) { return handle.id != UINT32_MAX; }

vkapi_res_cache_t* vkapi_res_cache_init(arena_t* arena)
{
    vkapi_res_cache_t* i = ARENA_MAKE_STRUCT(arena, vkapi_res_cache_t, ARENA_ZERO_MEMORY);
    MAKE_DYN_ARRAY(vkapi_texture_t, arena, 100, &i->textures);
    MAKE_DYN_ARRAY(vkapi_texture_t, arena, 100, &i->textures_gc);
    MAKE_DYN_ARRAY(texture_handle_t, arena, 100, &i->free_tex_slots);
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
