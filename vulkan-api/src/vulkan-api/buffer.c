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
    VkBufferUsageFlags usage)
{
    buffer->size = buff_size;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = buff_size;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | usage;

    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.flags =
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VMA_CHECK_RESULT(vmaCreateBuffer(
        vma_alloc,
        &bufferInfo,
        &allocCreateInfo,
        &buffer->buffer,
        &buffer->mem,
        &buffer->alloc_info));
}

void vkapi_buffer_map_to_stage(void* data, size_t data_size, vkapi_staging_instance_t* stage)
{
    assert(data);
    memcpy(stage->alloc_info.pMappedData, data, data_size);
}

void vkapi_buffer_map_to_gpu_buffer(vkapi_buffer_t* buffer, void* data, size_t data_size, size_t offset)
{
    assert(buffer);
    assert(data);
    memcpy((uint8_t*)buffer->alloc_info.pMappedData + offset, data, data_size);
}

void vkapi_map_and_copy_to_gpu(
    vkapi_buffer_t* buffer,
    vkapi_driver_t* driver,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    void* data)
{
    vkapi_staging_instance_t* stage =
        vkapi_staging_get(driver->staging_pool, driver->vma_allocator, size);
    vkapi_buffer_map_to_stage(data, size, stage);
    vkapi_copy_staged_to_gpu(buffer, driver, size, stage, usage);
}

void vkapi_copy_staged_to_gpu(
    vkapi_buffer_t* buffer,
    vkapi_driver_t* driver,
    VkDeviceSize size,
    vkapi_staging_instance_t* stage,
    VkBufferUsageFlags usage)
{
    // copy from the staging area to the allocated GPU memory
    vkapi_commands_t* cmds = driver->commands;
    vkapi_cmdbuffer_t* cmd = vkapi_commands_get_cmdbuffer(driver->context, cmds);

    VkBufferCopy copy_region = {.dstOffset = 0, .srcOffset = 0, .size = size};
    vkCmdCopyBuffer(cmd->instance, stage->buffer, buffer->buffer, 1, &copy_region);

    VkBufferMemoryBarrier mem_barrier = {};
    mem_barrier.buffer = buffer->buffer;
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

    vkapi_commands_t* cmds = driver->commands;
    vkapi_cmdbuffer_t* cmd = vkapi_commands_get_cmdbuffer(driver->context, cmds);

    VkMemoryBarrier mem_barrier = {
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT, .dstAccessMask = VK_ACCESS_HOST_READ_BIT};
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
    vkapi_commands_flush(driver->context, cmds);
    // TODO: Make thread blocking by this function optional.
    VK_CHECK_RESULT(vkWaitForFences(
       driver->context->device, 1, &cmd->fence, VK_TRUE, UINT64_MAX));

    memcpy(host_buffer, buffer->alloc_info.pMappedData, data_size);
}

void vkapi_buffer_destroy(vkapi_buffer_t* buffer, VmaAllocator vma_alloc)
{
    assert(buffer);
    vmaDestroyBuffer(vma_alloc, buffer->buffer, buffer->mem);
}

void vkapi_vert_buffer_create(
    vkapi_buffer_t* buffer,
    vkapi_driver_t* driver,
    void* data,
    VkDeviceSize data_size)
{
    assert(buffer);
    assert(driver);
    assert(data);

    // get a staging pool for hosting on the CPU side
    vkapi_staging_instance_t* stage =
        vkapi_staging_get(driver->staging_pool, driver->vma_allocator, data_size);

    vkapi_buffer_map_to_stage(data, data_size, stage);
    vkapi_buffer_alloc(buffer, driver->vma_allocator, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, data_size);
    vkapi_copy_staged_to_gpu(buffer, driver, data_size, stage, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

void vkapi_index_buffer_create(
    vkapi_buffer_t* buffer,
    vkapi_driver_t* driver,
    void* data,
    VkDeviceSize data_size)
{
    assert(buffer);
    assert(driver);
    assert(data);

    // get a staging pool for hosting on the CPU side
    vkapi_staging_instance_t* stage =
        vkapi_staging_get(driver->staging_pool, driver->vma_allocator, data_size);
    assert(stage);

    vkapi_buffer_map_to_stage(data, data_size, stage);
    vkapi_buffer_alloc(buffer, driver->vma_allocator, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, data_size);
    vkapi_copy_staged_to_gpu(buffer, driver, data_size, stage, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}


