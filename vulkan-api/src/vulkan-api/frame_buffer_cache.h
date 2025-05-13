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

#ifndef __VKAPI_FRAME_BUFFER_CACHE_H__
#define __VKAPI_FRAME_BUFFER_CACHE_H__

#include "backend/enums.h"
#include "common.h"
#include "pipeline_cache.h"
#include "renderpass.h"

#include <utility/hash.h>
#include <utility/hash_set.h>

typedef struct RPassKey
{
    VkImageLayout initial_layout[VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT];
    VkImageLayout final_layout[VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT];
    VkFormat colour_formats[VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT];
    enum LoadClearFlags load_op[VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT];
    enum StoreClearFlags store_op[VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT];
    enum LoadClearFlags ds_load_op[2];
    enum StoreClearFlags ds_store_op[2];
    VkFormat depth;
    uint32_t samples;
    uint32_t multi_view_count;
    uint8_t padding[3];
} vkapi_rpass_key_t;

typedef struct FboKey
{
    VkRenderPass renderpass;
    VkImageView views[VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT];
    uint32_t width;
    uint32_t height;
    uint16_t samples;
    uint16_t layer;
    uint32_t padding;
} vkapi_fbo_key_t;

typedef struct FrameBufferCache
{
    hash_set_t render_passes;
    hash_set_t fbos;

} vkapi_fb_cache_t;

vkapi_rpass_key_t vkapi_rpass_key_init();

vkapi_fbo_key_t vkapi_fbo_key_init();

vkapi_fb_cache_t* vkapi_fb_cache_init(arena_t* arena);

vkapi_rpass_t* vkapi_fb_cache_find_or_create_rpass(
    vkapi_fb_cache_t* cache, vkapi_rpass_key_t* key, vkapi_driver_t* driver, arena_t* arena);

vkapi_fbo_t* vkapi_fb_cache_find_or_create_fbo(
    vkapi_fb_cache_t* cache, vkapi_fbo_key_t* key, uint32_t count, vkapi_driver_t* driver);

void vkapi_fb_cache_gc(vkapi_fb_cache_t* c, vkapi_driver_t* driver, uint64_t current_frame);

void vkapi_fb_cache_destroy(vkapi_fb_cache_t* c, vkapi_driver_t* driver);

#endif