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

#ifndef __RPE_PRIV_VERTEX_BUFFER_H__
#define __RPE_PRIV_VERTEX_BUFFER_H__

#include "managers/renderable_manager.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <vulkan-api/resource_cache.h>

#define RPE_VERTEX_GPU_BUFFER_SIZE (1 << 15)
#define RPE_INDEX_GPU_BUFFER_SIZE (1 << 15)

typedef struct VkApiStageInstance vkapi_staging_instance_t;
typedef struct VkApiBuffer vkapi_buffer_t;

typedef struct VertexAllocInfo
{
    uint32_t offset;
    uint8_t* memory_ptr;
    uint32_t size;
} rpe_vertex_alloc_info_t;

typedef struct VertexBuffer
{
    // Size of vertex and index buffers (as a count of elements).
    uint32_t curr_vertex_size;
    uint32_t curr_index_size;

    // GPU buffers - uploaded to via a staging buffer.
    buffer_handle_t vertex_buffer;
    buffer_handle_t index_buffer;

    // Host vertex and indices data.
    rpe_vertex_t* vertex_data;
    uint32_t* index_data;

    // Any changes to the buffer are signalled by this flag - will lead to a GPU upload.
    bool is_dirty;

} rpe_vertex_buffer_t;

rpe_vertex_buffer_t* rpe_vertex_buffer_init(vkapi_driver_t* driver, arena_t* arena);

void rpe_vertex_buffer_copy_vert_data(
    rpe_vertex_buffer_t* vb, rpe_vertex_alloc_info_t alloc_info, rpe_vertex_t* data);

void rpe_vertex_buffer_copy_index_data_u32(
    rpe_vertex_buffer_t* vb, rpe_vertex_alloc_info_t alloc_info, const int32_t* data);

void rpe_vertex_buffer_copy_index_data_u16(
    rpe_vertex_buffer_t* vb, rpe_vertex_alloc_info_t alloc_info, const int16_t* data);

rpe_vertex_alloc_info_t rpe_vertex_buffer_alloc_vertex_buffer(rpe_vertex_buffer_t* vb, size_t size);

rpe_vertex_alloc_info_t rpe_vertex_buffer_alloc_index_buffer(rpe_vertex_buffer_t* vb, size_t size);

void rpe_vertex_buffer_upload_to_gpu(rpe_vertex_buffer_t* vb, vkapi_driver_t* driver);

#endif