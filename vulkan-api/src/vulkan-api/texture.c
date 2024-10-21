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

#include "texture.h"

#include "commands.h"
#include "driver.h"
#include "utility.h"

#include <assert.h>
#include <string.h>

uint32_t vkapi_texture_format_comp_size(VkFormat format)
{
    uint32_t comp = 0;
    switch (format)
    {
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8_SRGB:
        case VK_FORMAT_R8_SSCALED:
        case VK_FORMAT_R16_SNORM:
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16_SSCALED:
        case VK_FORMAT_R16_SFLOAT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32_SFLOAT:
            comp = 1;
            break;
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R8G8_SRGB:
        case VK_FORMAT_R8G8_SSCALED:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16_SSCALED:
        case VK_FORMAT_R16G16_SFLOAT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32_SFLOAT:
            comp = 2;
            break;
        case VK_FORMAT_R8G8B8_SNORM:
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_R8G8B8_SSCALED:
        case VK_FORMAT_R16G16B16_SNORM:
        case VK_FORMAT_R16G16B16_SINT:
        case VK_FORMAT_R16G16B16_SSCALED:
        case VK_FORMAT_R16G16B16_SFLOAT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32_SFLOAT:
            comp = 3;
            break;
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_R8G8B8A8_SSCALED:
        case VK_FORMAT_R16G16B16A16_SNORM:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R16G16B16A16_SSCALED:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            comp = 4;
            break;
        default:
            comp = UINT32_MAX;
    }
    return comp;
}

uint32_t vkapi_texture_format_byte_size(VkFormat format)
{
    uint32_t output = 1;
    switch (format)
    {
        case VK_FORMAT_R8_SNORM:
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8_SRGB:
        case VK_FORMAT_R8_SSCALED:
        case VK_FORMAT_R8G8_SNORM:
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R8G8_SRGB:
        case VK_FORMAT_R8G8_SSCALED:
        case VK_FORMAT_R8G8B8_SNORM:
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_R8G8B8_SSCALED:
        case VK_FORMAT_R8G8B8A8_SNORM:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_R8G8B8A8_SSCALED:
            output = 1;
            break;
        case VK_FORMAT_R16_SNORM:
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16_SSCALED:
        case VK_FORMAT_R16_SFLOAT:
        case VK_FORMAT_R16G16_SNORM:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16_SSCALED:
        case VK_FORMAT_R16G16_SFLOAT:
        case VK_FORMAT_R16G16B16_SNORM:
        case VK_FORMAT_R16G16B16_SINT:
        case VK_FORMAT_R16G16B16_SSCALED:
        case VK_FORMAT_R16G16B16_SFLOAT:
        case VK_FORMAT_R16G16B16A16_SNORM:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R16G16B16A16_SSCALED:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            output = 4;
            break;
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R32_SFLOAT:
        case VK_FORMAT_R32G32_SFLOAT:
        case VK_FORMAT_R32G32B32_SFLOAT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            output = 8;
            break;
        default:
            output = UINT32_MAX;
            break;
    }
    return output;
}

vkapi_texture_t vkapi_texture_init(
    uint32_t width,
    uint32_t height,
    uint32_t mip_levels,
    uint32_t face_count,
    uint32_t array_count,
    VkFormat format)
{
    assert(width > 0 && height > 0);
    assert(mip_levels > 0 && mip_levels < VKAPI_TEXTURE_MAX_MIP_COUNT);
    assert(face_count >= 1 && face_count <= 6);

    vkapi_texture_t tex = {
        .info.width = width,
        .info.height = height,
        .info.mip_levels = mip_levels,
        .info.face_count = face_count,
        .info.array_count = array_count,
        .info.format = format,
        .image_layout = VK_IMAGE_LAYOUT_UNDEFINED,
        .image = VK_NULL_HANDLE,
        .image_memory = VK_NULL_HANDLE,
        .frames_until_gc = 0};

    return tex;
}

void vkapi_texture_destroy(vkapi_context_t* context, vkapi_texture_t* texture)
{
    vkFreeMemory(context->device, texture->image_memory, VK_NULL_HANDLE);
    vkDestroyImage(context->device, texture->image, VK_NULL_HANDLE);
    for (uint32_t level = 0; level < texture->info.mip_levels; ++level)
    {
        vkDestroyImageView(context->device, texture->image_views[level], VK_NULL_HANDLE);
    }
}

void vkapi_texture_create_image(
    vkapi_context_t* context, vkapi_texture_t* texture, VkImageUsageFlags usage_flags)
{
    assert(texture->info.format != VK_FORMAT_UNDEFINED);

    VkImageCreateInfo image_info = {0};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D; // TODO: support 3d images
    image_info.format = texture->info.format;
    image_info.extent.width = texture->info.width;
    image_info.extent.height = texture->info.height;
    image_info.extent.depth = 1;
    image_info.mipLevels = texture->info.mip_levels;
    image_info.arrayLayers = texture->info.face_count;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | usage_flags;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (texture->info.face_count == 6)
    {
        image_info.arrayLayers = 6;
        image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    VK_CHECK_RESULT(vkCreateImage(context->device, &image_info, VK_NULL_HANDLE, &texture->image));

    // allocate memory for the image....
    VkMemoryRequirements mem_req = {0};
    vkGetImageMemoryRequirements(context->device, texture->image, &mem_req);
    VkMemoryAllocateInfo alloc_info = {0};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_req.size;
    alloc_info.memoryTypeIndex = vkapi_context_select_mem_type(
        context, mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VK_CHECK_RESULT(
        vkAllocateMemory(context->device, &alloc_info, VK_NULL_HANDLE, &texture->image_memory));

    // and bind the image to the allocated memory.
    vkBindImageMemory(context->device, texture->image, texture->image_memory, 0);
}

void vkapi_texture_create_image_view(
    vkapi_context_t* context, vkapi_texture_t* texture, int view_idx)
{
    // Work out the image view type.
    VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D;
    if (texture->info.array_count > 1 && texture->info.face_count == 1)
    {
        view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    }
    else if (texture->info.face_count == 6)
    {
        view_type = (texture->info.array_count == 1) ? VK_IMAGE_VIEW_TYPE_CUBE
                                                     : VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    }

    // making assumptions here based on the image format used.
    VkImageAspectFlags aspect = vkapi_texture_aspect_flags(texture->info.format);
    switch (texture->info.format)
    {
        // depth/stencil image formats
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
            aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            break;
            // depth only formats
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D16_UNORM:
            aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
            break;
            // otherwise, must be a colour format
        default:
            aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkImageViewCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = texture->image;
    create_info.viewType = view_type;
    create_info.format = texture->info.format;
    create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.subresourceRange.layerCount = texture->info.face_count;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.aspectMask = aspect;

    VK_CHECK_RESULT(vkCreateImageView(
        context->device, &create_info, VK_NULL_HANDLE, &texture->image_views[view_idx]));
}

void vkapi_texture_create_2d(
    vkapi_context_t* context, vkapi_texture_t* texture, VkImageUsageFlags usage_flags)
{
    assert(
        texture->info.mip_levels < VKAPI_TEXTURE_MAX_MIP_COUNT &&
        "Requested mip level exceed max allowed count");

    // create an empty image
    vkapi_texture_create_image(context, texture, usage_flags);

    // and a image view for each mip level
    for (uint32_t level = 0; level < texture->info.mip_levels; ++level)
    {
        vkapi_texture_create_image_view(context, texture, level);
    }

    texture->image_layout =
        (vkapi_util_is_depth(texture->info.format) || vkapi_util_is_stencil(texture->info.format))
        ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL
        : (usage_flags & VK_IMAGE_USAGE_STORAGE_BIT) ? VK_IMAGE_LAYOUT_GENERAL
                                                     : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void vkapi_texture_map(
    vkapi_context_t* context,
    vkapi_staging_pool_t* staging_pool,
    vkapi_commands_t* commands,
    VmaAllocator* vma_alloc,
    vkapi_texture_t* texture,
    void* data,
    uint32_t data_size,
    size_t* offsets,
    arena_t* scratch_arena)
{
    vkapi_staging_instance_t* stage = vkapi_staging_get(staging_pool, vma_alloc, data_size);

    memcpy(stage->alloc_info.pMappedData, data, data_size);
    vmaFlushAllocation(*vma_alloc, stage->mem, 0, data_size);

    if (!offsets)
    {
        offsets = ARENA_MAKE_ARRAY(
            scratch_arena, size_t, texture->info.face_count * texture->info.mip_levels, 0);
        uint32_t offset = 0;
        for (uint32_t face = 0; face < texture->info.face_count; face++)
        {
            for (uint32_t level = 0; level < texture->info.mip_levels; ++level)
            {
                offsets[face * texture->info.mip_levels + level] = offset;
                offset += (texture->info.width >> level) * (texture->info.height >> level) *
                    vkapi_texture_format_comp_size(texture->info.format) *
                    vkapi_texture_format_byte_size(texture->info.format);
            }
        }
    }

    // Create the info required for the copy.
    VkBufferImageCopy* copy_buffers = ARENA_MAKE_ARRAY(
        scratch_arena, VkBufferImageCopy, texture->info.face_count * texture->info.mip_levels, 0);
    for (uint32_t face = 0; face < texture->info.face_count; ++face)
    {
        for (uint32_t level = 0; level < texture->info.mip_levels; ++level)
        {
            VkBufferImageCopy image_copy = {0};
            image_copy.bufferOffset = offsets[face * texture->info.mip_levels + level];
            image_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_copy.imageSubresource.mipLevel = level;
            image_copy.imageSubresource.layerCount = face;
            image_copy.imageSubresource.baseArrayLayer = 1;
            image_copy.imageExtent.width = texture->info.width >> level;
            image_copy.imageExtent.height = texture->info.height >> level;
            image_copy.imageExtent.depth = 1;
            copy_buffers[face * texture->info.mip_levels + level] = image_copy;
        }
    }

    // now copy image to local device - first prepare the image for copying via
    // transitioning to a transfer state. After copying, the image is
    // transistioned ready for reading by the shader
    vkapi_cmdbuffer_t* cmds = vkapi_commands_get_cmdbuffer(context, commands);

    vkapi_texture_image_transition(
        texture,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        cmds->instance,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        UINT32_MAX);

    vkCmdCopyBufferToImage(
        cmds->instance,
        stage->buffer,
        texture->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        texture->info.face_count * texture->info.mip_levels,
        copy_buffers);

    vkapi_texture_image_transition(
        texture,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        texture->image_layout,
        cmds->instance,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        UINT32_MAX);
}

void vkapi_texture_image_transition(
    vkapi_texture_t* texture,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    VkCommandBuffer cmd_buffer,
    VkPipelineStageFlags srcStage,
    VkPipelineStageFlags dstStage,
    uint32_t baseMipMapLevel)
{
    VkImageAspectFlags mask = vkapi_texture_aspect_flags(texture->info.format);
    VkAccessFlags srcBarrier, dstBarrier;

    switch (old_layout)
    {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            srcBarrier = 0;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            srcBarrier = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            srcBarrier = VK_ACCESS_TRANSFER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            srcBarrier = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            srcBarrier = VK_ACCESS_SHADER_READ_BIT;
            break;
        default:
            srcBarrier = 0;
    }

    switch (new_layout)
    {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            dstBarrier = VK_ACCESS_TRANSFER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            dstBarrier = VK_ACCESS_TRANSFER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            dstBarrier = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            dstBarrier = VK_ACCESS_SHADER_READ_BIT;
            break;
        case VK_IMAGE_LAYOUT_GENERAL:
            dstBarrier = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            dstBarrier = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            break;
        default:
            dstBarrier = 0;
    }

    VkImageSubresourceRange subresourceRange = {0};
    subresourceRange.aspectMask = mask;
    subresourceRange.levelCount = 0;
    subresourceRange.layerCount = texture->info.array_count * texture->info.face_count;
    subresourceRange.baseMipLevel = texture->info.mip_levels;
    subresourceRange.baseArrayLayer = 0;

    if (baseMipMapLevel != UINT32_MAX)
    {
        subresourceRange.baseMipLevel = baseMipMapLevel;
        subresourceRange.levelCount = 1;
    }

    VkImageMemoryBarrier mem_barrier = {0};
    mem_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    mem_barrier.image = texture->image;
    mem_barrier.oldLayout = old_layout;
    mem_barrier.newLayout = new_layout;
    mem_barrier.subresourceRange = subresourceRange;
    mem_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mem_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mem_barrier.srcAccessMask = srcBarrier;
    mem_barrier.dstAccessMask = dstBarrier;

    vkCmdPipelineBarrier(
        cmd_buffer, srcStage, dstStage, 0, 0, VK_NULL_HANDLE, 0, VK_NULL_HANDLE, 1, &mem_barrier);

    texture->image_layout = new_layout;
}

void vkapi_texture_blit(
    vkapi_texture_t* src_texture,
    vkapi_texture_t* dst_texture,
    vkapi_context_t* context,
    vkapi_commands_t* commands)
{
    // source
    VkCommandBuffer cmds = vkapi_commands_get_cmdbuffer(context, commands)->instance;
    VkImageAspectFlags image_aspect = vkapi_texture_aspect_flags(src_texture->info.format);

    VkImageSubresourceLayers src_subres = {0};
    src_subres.aspectMask = image_aspect;
    src_subres.mipLevel = 0;
    src_subres.baseArrayLayer = 0;
    src_subres.layerCount = 1;

    VkOffset3D src_offset = {0};
    src_offset.x = src_texture->info.width;
    src_offset.y = src_texture->info.height;
    src_offset.z = 1;

    // destination
    VkImageSubresourceLayers dst_subres = {0};
    dst_subres.aspectMask = image_aspect;
    dst_subres.mipLevel = 0;
    dst_subres.baseArrayLayer = 0;
    dst_subres.layerCount = 1;

    VkOffset3D dst_offset = {0};
    dst_offset.x = dst_texture->info.width;
    dst_offset.y = dst_texture->info.height;
    dst_offset.z = 1;

    VkImageBlit image_blit;
    image_blit.srcSubresource = src_subres;
    image_blit.srcOffsets[1] = src_offset;
    image_blit.dstSubresource = dst_subres;
    image_blit.dstOffsets[1] = dst_offset;

    vkapi_texture_image_transition(
        src_texture,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        cmds,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        UINT32_MAX);

    vkapi_texture_image_transition(
        dst_texture,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        cmds,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        UINT32_MAX);

    // blit the image
    VkFilter filter = vkapi_texture_filter_type(src_texture->info.format);
    vkCmdBlitImage(
        cmds,
        dst_texture->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        src_texture->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &image_blit,
        filter);

    vkapi_texture_image_transition(
        src_texture,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        cmds,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        UINT32_MAX);

    vkapi_texture_image_transition(
        dst_texture,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
        cmds,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        UINT32_MAX);
}

VkFilter vkapi_texture_filter_type(VkFormat format)
{
    VkFilter filter;

    if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT ||
        format == VK_FORMAT_D32_SFLOAT || format == VK_FORMAT_D16_UNORM)
    {
        filter = VK_FILTER_NEAREST;
    }
    else
    {
        filter = VK_FILTER_LINEAR;
    }
    return filter;
}

VkImageAspectFlags vkapi_texture_aspect_flags(VkFormat format)
{
    VkImageAspectFlags aspect;
    switch (format)
    {
        // depth/stencil image formats
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
            aspect = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            break;
            // depth only formats
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D16_UNORM:
            aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
            break;
            // otherwise, must be a colour format
        default:
            aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    return aspect;
}
