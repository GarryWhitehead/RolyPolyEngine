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

#include "buffer.h"

#include "commands.h"
#include "common.h"
#include "context.h"
#include "driver.h"
#include "staging_pool.h"

vkapi_buffer_t vkapi_buffer_init()
{
    vkapi_buffer_t b;
    memset(&b, 0, sizeof(vkapi_buffer_t));
    return b;
}

void vkapi_buffer_alloc(
    vkapi_buffer_t* buffer,
    VmaAllocator vma_alloc,
    VkDeviceSize buff_size,
    VkBufferUsageFlags usage,
    enum BufferType type)
{
    buffer->size = buff_size;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = buff_size;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | usage;

    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    if (type == VKAPI_BUFFER_HOST_TO_GPU)
    {
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }
    else if (type == VKAPI_BUFFER_GPU_TO_HOST)
    {
        allocCreateInfo.flags =
            VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }
    else if (type == VKAPI_BUFFER_GPU_ONLY)
    {
        allocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    }

    VMA_CHECK_RESULT(vmaCreateBuffer(
        vma_alloc,
        &bufferInfo,
        &allocCreateInfo,
        &buffer->buffer,
        &buffer->mem,
        &buffer->alloc_info));
}

void vkapi_buffer_map_to_gpu_buffer(
    vkapi_buffer_t* buffer, void* data, size_t data_size, size_t offset)
{
    assert(buffer);
    assert(data);
    memcpy((uint8_t*)buffer->alloc_info.pMappedData + offset, data, data_size);
}

void vkapi_map_and_copy_to_gpu(
    vkapi_driver_t* driver,
    vkapi_buffer_t* dst_buffer,
    VkDeviceSize size,
    VkDeviceSize offset,
    VkBufferUsageFlags usage,
    void* data)
{
    vkapi_staging_instance_t* stage =
        vkapi_staging_get(driver->staging_pool, driver->vma_allocator, size);

    void* mapped;
    vmaMapMemory(driver->vma_allocator, stage->mem, &mapped);
    assert(mapped);
    memcpy(mapped, data, size);
    vmaUnmapMemory(driver->vma_allocator, stage->mem);
    vmaFlushAllocation(driver->vma_allocator, stage->mem, offset, size);

    vkapi_copy_staged_to_gpu(driver, size, stage, dst_buffer, offset, offset, usage);
}

void vkapi_copy_staged_to_gpu(
    vkapi_driver_t* driver,
    VkDeviceSize size,
    vkapi_staging_instance_t* stage,
    vkapi_buffer_t* dst_buffer,
    uint32_t src_offset,
    uint32_t dst_offset,
    VkBufferUsageFlags usage)
{
    // copy from the staging area to the allocated GPU memory
    vkapi_commands_t* cmds = driver->commands;
    vkapi_cmdbuffer_t* cmd = vkapi_commands_get_cmdbuffer(driver->context, cmds);

    VkBufferCopy copy_region = {.dstOffset = dst_offset, .srcOffset = src_offset, .size = size};
    vkCmdCopyBuffer(cmd->instance, stage->buffer, dst_buffer->buffer, 1, &copy_region);

    VkBufferMemoryBarrier mem_barrier = {0};
    mem_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    mem_barrier.buffer = stage->buffer;
    mem_barrier.size = VK_WHOLE_SIZE;

    // ensure that the copy finishes before the next frames draw call
    if (usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT || usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
    {
        mem_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        mem_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT |
            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT;

        vkCmdPipelineBarrier(
            cmd->instance,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            0,
            0,
            VK_NULL_HANDLE,
            1,
            &mem_barrier,
            0,
            VK_NULL_HANDLE);
    }
    else if (usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
    {
        mem_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        mem_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_UNIFORM_READ_BIT;

        vkCmdPipelineBarrier(
            cmd->instance,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0,
            0,
            VK_NULL_HANDLE,
            1,
            &mem_barrier,
            0,
            VK_NULL_HANDLE);
    }
    else if (usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
    {
        mem_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        mem_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(
            cmd->instance,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0,
            0,
            VK_NULL_HANDLE,
            1,
            &mem_barrier,
            0,
            VK_NULL_HANDLE);
    }
}

void vkapi_buffer_download_to_host(
    vkapi_buffer_t* buffer, vkapi_driver_t* driver, void* host_buffer, size_t data_size)
{
    assert(buffer);
    assert(driver);
    assert(host_buffer);
    assert(data_size > 0);

    vkapi_cmdbuffer_t* cmd =
        vkapi_commands_get_cmdbuffer(driver->context, driver->compute_commands);

    VkMemoryBarrier mem_barrier = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_HOST_READ_BIT};
    vkCmdPipelineBarrier(
        cmd->instance,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_HOST_BIT,
        0,
        1,
        &mem_barrier,
        0,
        VK_NULL_HANDLE,
        0,
        VK_NULL_HANDLE);

    vkapi_commands_flush(driver->context, driver->compute_commands);
    VK_CHECK_RESULT(vkWaitForFences(driver->context->device, 1, &cmd->fence, VK_TRUE, UINT64_MAX))
    memcpy(host_buffer, buffer->alloc_info.pMappedData, data_size);
}

void vkapi_buffer_destroy(vkapi_buffer_t* buffer, VmaAllocator vma_alloc)
{
    assert(buffer);
    vmaDestroyBuffer(vma_alloc, buffer->buffer, buffer->mem);
}

void vkapi_buffer_upload_vertex_data(
    vkapi_buffer_t* dst_buffer,
    vkapi_driver_t* driver,
    void* data,
    VkDeviceSize data_size,
    uint32_t buffer_offset)
{
    assert(dst_buffer);
    assert(driver);
    assert(data);

    // TODO: If copying staging to GPU buffer, the first vertex is not copied for some reason. Need
    // to investigate why, for now using the slower method of mapping to the buffer directly.
    // vkapi_map_and_copy_to_gpu(driver, dst_buffer, data_size, buffer_offset,
    // VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, data);
    memcpy(dst_buffer->alloc_info.pMappedData, data, data_size);
}

void vkapi_buffer_upload_index_data(
    vkapi_buffer_t* dst_buffer,
    vkapi_driver_t* driver,
    void* data,
    VkDeviceSize data_size,
    uint32_t buffer_offset)
{
    assert(dst_buffer);
    assert(driver);
    assert(data);

    vkapi_map_and_copy_to_gpu(
        driver, dst_buffer, data_size, buffer_offset, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, data);
}
