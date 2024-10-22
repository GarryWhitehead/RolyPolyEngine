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

#ifndef __VKAPI_RESOURCE_CACHE_H__
#define __VKAPI_RESOURCE_CACHE_H__

#include <stdbool.h>
#include <utility/arena.h>
#include <vulkan-api/common.h>

// Forward declarations.
typedef struct VkApiContext vkapi_context_t;

typedef struct TextureHandle
{
    uint32_t id;
} texture_handle_t;

typedef struct BufferHandle
{
    uint32_t id;
} buffer_handle_t;

typedef struct VkApiResourceCache
{
    arena_dyn_array_t textures;
    arena_dyn_array_t free_tex_slots;

    arena_dyn_array_t textures_gc;
} vkapi_res_cache_t;

bool vkapi_tex_handle_is_valid(texture_handle_t handle);

vkapi_res_cache_t* vkapi_res_cache_init(arena_t* arena);

texture_handle_t vkapi_res_cache_create_tex2d(
    vkapi_res_cache_t* cache,
    vkapi_context_t* context,
    VkFormat format,
    uint32_t width,
    uint32_t height,
    uint8_t mip_levels,
    uint8_t face_count,
    uint8_t array_count,
    VkImageUsageFlags usage_flags);

void vkapi_res_cache_delete_tex2d(vkapi_res_cache_t* cache, texture_handle_t handle);

#endif