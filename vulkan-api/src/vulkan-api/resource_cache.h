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

#include "buffer.h"
#include "common.h"
#include "texture.h"

#include <stdbool.h>
#include <utility/arena.h>

// 3 swapchain image textures and 1 final depth image.
#define VKAPI_RES_CACHE_MAX_RESERVED_COUNT 4

// Forward declarations.
typedef struct VkApiContext vkapi_context_t;
typedef struct VkApiDriver vkapi_driver_t;
typedef struct VkApiBuffer vkapi_buffer_t;
typedef struct Texture vkapi_texture_t;

#define VKAPI_INVALID_TEXTURE_HANDLE UINT32_MAX
#define VKAPI_INVALID_BUFFER_HANDLE UINT32_MAX

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

    arena_dyn_array_t buffers;
    arena_dyn_array_t free_buffer_slots;

    arena_dyn_array_t textures_gc;
    arena_dyn_array_t buffers_gc;
} vkapi_res_cache_t;

bool vkapi_tex_handle_is_valid(texture_handle_t handle);
void vkapi_invalidate_tex_handle(texture_handle_t* handle);
bool vkapi_buffer_handle_is_valid(buffer_handle_t handle);
void vkapi_invalidate_buffer_handle(buffer_handle_t* handle);

vkapi_res_cache_t* vkapi_res_cache_init(vkapi_driver_t* driver, arena_t* arena);

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
    sampler_params_t* sampler_params);

texture_handle_t vkapi_res_push_reserved_tex2d(
    vkapi_res_cache_t* cache,
    vkapi_context_t* context,
    uint32_t width,
    uint32_t height,
    VkFormat format,
    uint32_t idx,
    VkImageUsageFlags usage_flags,
    VkImage* image);

vkapi_texture_t* vkapi_res_cache_get_tex2d(vkapi_res_cache_t* cache, texture_handle_t handle);

buffer_handle_t
vkapi_res_cache_create_ubo(vkapi_res_cache_t* cache, vkapi_driver_t* driver, VkDeviceSize size);

buffer_handle_t vkapi_res_cache_create_ssbo(
    vkapi_res_cache_t* cache,
    vkapi_driver_t* driver,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    enum BufferType type);

buffer_handle_t vkapi_res_cache_create_vertex_buffer(
    vkapi_res_cache_t* cache, vkapi_driver_t* driver, VkDeviceSize size);

buffer_handle_t vkapi_res_cache_create_index_buffer(
    vkapi_res_cache_t* cache, vkapi_driver_t* driver, VkDeviceSize size);

vkapi_buffer_t* vkapi_res_cache_get_buffer(vkapi_res_cache_t* cache, buffer_handle_t handle);

void vkapi_res_cache_delete_tex2d(vkapi_res_cache_t* cache, texture_handle_t handle);

void vkapi_res_cache_gc(vkapi_res_cache_t* c, vkapi_driver_t* driver);
void vkapi_res_cache_destroy(vkapi_res_cache_t* c, vkapi_driver_t* driver);

#endif