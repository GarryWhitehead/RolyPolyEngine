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

#ifndef __VKAPI_TEXTURE_H__
#define __VKAPI_TEXTURE_H__

#include "common.h"

#include <utility/compiler.h>

// Forward declarations.
typedef struct VkApiContext vkapi_context_t;
typedef struct VkApiDriver vkapi_driver_t;
typedef struct Arena arena_t;
typedef struct Commands vkapi_commands_t;
typedef struct VkApiStagingPool vkapi_staging_pool_t;
typedef struct TextureSamplerParams sampler_params_t;
typedef struct SamplerCache vkapi_sampler_cache_t;

#define VKAPI_TEXTURE_MAX_MIP_COUNT 12

enum TextureType
{
    VKAPI_TEXTURE_2D,
    VKAPI_TEXTURE_2D_ARRAY,
    VKAPI_TEXTURE_2D_CUBE,
    VKAPI_TEXTURE_2D_CUBE_ARRAY
};

typedef struct Texture
{
    struct ImageInfo
    {
        VkFormat format;
        uint32_t width;
        uint32_t height;
        uint32_t mip_levels;
        uint32_t array_count;
        enum TextureType type;
    } info;

    VkImageLayout image_layout;
    VkImage image;
    VmaAllocation vma_alloc;

    // Includes omage views for using different mip levels as render targets.
    VkImageView image_views[VKAPI_TEXTURE_MAX_MIP_COUNT];

    // Special image view for framebuffer rendertarget - single mip level declared.
    VkImageView framebuffer_imageview;

    // Note: The sampler cache holds the main reference to the sampler and will destroy on
    // termination.
    VkSampler sampler;
    uint64_t frames_until_gc;
    bool is_valid;

} vkapi_texture_t;

vkapi_texture_t vkapi_texture_init(
    uint32_t width,
    uint32_t height,
    uint32_t mip_levels,
    uint32_t array_count,
    enum TextureType type,
    VkFormat format);

void vkapi_texture_destroy(vkapi_context_t* context, VmaAllocator vma, vkapi_texture_t* texture);

uint32_t vkapi_texture_format_byte_size(VkFormat format);

uint32_t vkapi_texture_format_comp_size(VkFormat format);

void vkapi_texture_create_image(VmaAllocator vma, vkapi_texture_t* texture, VkImageUsageFlags usage_flags);

VkImageView vkapi_texture_create_image_view(
    vkapi_context_t* context, vkapi_texture_t* texture, uint32_t mip_level, uint32_t mip_count);

void vkapi_texture_create_2d(
    vkapi_context_t* context,
    VmaAllocator vma,
    vkapi_sampler_cache_t* sc,
    vkapi_texture_t* texture,
    VkImageUsageFlags usage_flags,
    sampler_params_t* sampler_params);

void vkapi_texture_map(
    vkapi_context_t* context,
    vkapi_staging_pool_t* staging_pool,
    vkapi_commands_t* commands,
    VmaAllocator vma_alloc,
    vkapi_texture_t* texture,
    void* data,
    uint32_t data_size,
    size_t* offsets,
    arena_t* scratch_arena,
    bool generate_mipmaps);

void vkapi_texture_image_transition(
    vkapi_texture_t* texture,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    VkCommandBuffer cmd_buffer,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage,
    uint32_t baseMipMapLevel);

void vkapi_texture_image_multi_transition(
    vkapi_texture_t* texture,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    VkCommandBuffer cmd_buffer,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage,
    uint32_t level_count);

void vkapi_texture_gen_mipmaps(
    vkapi_texture_t* tex, vkapi_context_t* context, vkapi_commands_t* commands, size_t level_count);

void vkapi_texture_blit(
    vkapi_texture_t* src_texture,
    vkapi_texture_t* dst_texture,
    vkapi_context_t* context,
    vkapi_commands_t* commands);

VkFilter vkapi_texture_filter_type(VkFormat format);

VkImageAspectFlags vkapi_texture_aspect_flags(VkFormat format);
VkPipelineStageFlags vkapi_texture_get_pline_stage_flag(VkImageLayout layout);

#endif