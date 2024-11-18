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

#ifndef __VKAPI_BUFFER_H__
#define __VKAPI_BUFFER_H__

#include "common.h"

typedef struct VkApiDriver vkapi_driver_t;
typedef struct Arena arena_t;
typedef struct VkApiStageInstance vkapi_staging_instance_t;
typedef struct VkApiStagingPool vkapi_staging_pool_t;

typedef struct VkApiBuffer
{
    VmaAllocationInfo alloc_info;
    VmaAllocation mem;
    VkDeviceSize size;
    VkBuffer buffer;
    uint32_t frames_until_gc;
} vkapi_buffer_t;

vkapi_buffer_t vkapi_buffer_init();

void vkapi_buffer_alloc(
    vkapi_buffer_t* buffer,
    VmaAllocator vma_alloc,
    VkDeviceSize buff_size,
    VkBufferUsageFlags usage);

void vkapi_buffer_map_to_stage(void* data, size_t data_size, vkapi_staging_instance_t* stage);

void vkapi_buffer_map_to_gpu_buffer(vkapi_buffer_t* buffer, void* data, size_t data_size, size_t offset);

void vkapi_map_and_copy_to_gpu(
    vkapi_buffer_t* buffer,
    vkapi_driver_t* driver,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    void* data);

void vkapi_copy_staged_to_gpu(
    vkapi_buffer_t* buffer,
    vkapi_driver_t* driver,
    VkDeviceSize size,
    vkapi_staging_instance_t* stage,
    VkBufferUsageFlags usage);

void vkapi_buffer_download_to_host(vkapi_buffer_t* buffer, vkapi_driver_t* driver, void* host_buffer, size_t data_size);

void vkapi_buffer_destroy(vkapi_buffer_t* buffer, VmaAllocator vma_alloc);

void vkapi_vert_buffer_create(
    vkapi_buffer_t* buffer,
    vkapi_driver_t* driver,
    void* data,
    VkDeviceSize data_size);

void vkapi_index_buffer_create(
    vkapi_buffer_t* buffer,
    vkapi_driver_t* driver,
    void* data,
    VkDeviceSize data_size);

#endif
