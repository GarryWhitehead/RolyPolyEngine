/* Copyright (c) 2022 Garry Whitehead
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

#pragma once

#define VKAPI_TEXTURE_MAX_MIP_COUNT 12

#include "common.h"
#include "utility/compiler.h"

// Forward declarations.
typedef struct VkApiContext vkapi_context_t;
typedef struct Arena arena_t;
typedef struct Commands vkapi_commands_t;
typedef struct StagingPool vkapi_staging_pool_t;

typedef struct Texture
{
    struct ImageInfo
    {
        VkFormat format;
        uint32_t width;
        uint32_t height;
        uint32_t mip_levels;
        uint32_t face_count;
        uint32_t array_count;
    } info;

    VkImageLayout image_layout;

    VkImage image;
    VkDeviceMemory image_memory;

    VkImageView image_views[VKAPI_TEXTURE_MAX_MIP_COUNT];

    uint64_t frames_until_gc;

} vkapi_texture_t;

vkapi_texture_t vkapi_texture_init(
    uint32_t width,
    uint32_t height,
    uint32_t mip_levels,
    uint32_t face_count,
    uint32_t array_count,
    VkFormat format);

void vkapi_texture_destroy(vkapi_context_t* context, vkapi_texture_t* texture);

uint32_t vkapi_texture_format_byte_size(VkFormat format);

uint32_t vkapi_texture_format_comp_size(VkFormat format);

void vkapi_texture_create_image(
    vkapi_context_t* context, vkapi_texture_t* texture, VkImageUsageFlags usage_flags);

void vkapi_texture_create_image_view(
    vkapi_context_t* context, vkapi_texture_t* texture, int view_idx);

void vkapi_texture_create_2d(
    vkapi_context_t* context, vkapi_texture_t* texture, VkImageUsageFlags usage_flags);

void vkapi_texture_map(
    vkapi_context_t* context,
    vkapi_staging_pool_t* staging_pool,
    vkapi_commands_t* commands,
    VmaAllocator* vma_alloc,
    vkapi_texture_t* texture,
    void* data,
    uint32_t data_size,
    size_t* offsets,
    arena_t* scratch_arena);

void vkapi_texture_image_transition(
    vkapi_texture_t* texture,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    VkCommandBuffer cmd_buffer,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage,
    uint32_t baseMipMapLevel);

void vkapi_texture_blit(
    vkapi_texture_t* src_texture,
    vkapi_texture_t* dst_texture,
    vkapi_context_t* context,
    vkapi_commands_t* commands);

VkFilter vkapi_texture_filter_type(VkFormat format);

VkImageAspectFlags vkapi_texture_aspect_flags(VkFormat format);
