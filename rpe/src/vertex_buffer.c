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

#include "vertex_buffer.h"

#include <string.h>
#include <utility/arena.h>
#include <vulkan-api/buffer.h>
#include <vulkan-api/driver.h>

rpe_vertex_buffer_t* rpe_vertex_buffer_init(vkapi_driver_t* driver, arena_t* arena)
{
    rpe_vertex_buffer_t* i = ARENA_MAKE_ZERO_STRUCT(arena, rpe_vertex_buffer_t);

    i->vertex_data =
        ARENA_MAKE_ARRAY(arena, rpe_vertex_t, RPE_VERTEX_GPU_BUFFER_SIZE, ARENA_ZERO_MEMORY);
    i->index_data = ARENA_MAKE_ARRAY(arena, uint32_t, RPE_INDEX_GPU_BUFFER_SIZE, ARENA_ZERO_MEMORY);

    i->vertex_buffer = vkapi_res_cache_create_vertex_buffer(
        driver->res_cache, driver, RPE_VERTEX_GPU_BUFFER_SIZE * sizeof(rpe_vertex_t));
    i->index_buffer = vkapi_res_cache_create_index_buffer(
        driver->res_cache, driver, RPE_INDEX_GPU_BUFFER_SIZE * sizeof(uint32_t));

    return i;
}

void rpe_vertex_buffer_copy_vert_data(
    rpe_vertex_buffer_t* vb, rpe_vertex_alloc_info_t alloc_info, rpe_vertex_t* data)
{
    assert(data);
    assert(alloc_info.size > 0);
    assert(alloc_info.memory_ptr);
    memcpy(alloc_info.memory_ptr, data, alloc_info.size * sizeof(rpe_vertex_t));
    vb->is_dirty = true;
}

rpe_vertex_alloc_info_t rpe_vertex_buffer_alloc_vertex_buffer(rpe_vertex_buffer_t* vb, size_t size)
{
    assert(vb);
    assert(vb->curr_vertex_size + size < RPE_VERTEX_GPU_BUFFER_SIZE);
    rpe_vertex_alloc_info_t i;
    i.memory_ptr = (uint8_t*)(vb->vertex_data + vb->curr_vertex_size);
    i.offset = vb->curr_vertex_size;
    i.size = size;
    vb->curr_vertex_size += size;
    return i;
}

rpe_vertex_alloc_info_t rpe_vertex_buffer_alloc_index_buffer(rpe_vertex_buffer_t* vb, size_t size)
{
    assert(vb);
    assert(vb->curr_index_size + size < RPE_INDEX_GPU_BUFFER_SIZE);
    rpe_vertex_alloc_info_t i;
    i.memory_ptr = (uint8_t*)(vb->index_data + vb->curr_index_size);
    i.offset = vb->curr_index_size;
    i.size = size;
    vb->curr_index_size += size;
    return i;
}

void rpe_vertex_buffer_copy_index_data_u32(
    rpe_vertex_buffer_t* vb, rpe_vertex_alloc_info_t alloc_info, const int32_t* data)
{
    assert(data);
    assert(alloc_info.memory_ptr);
    assert(alloc_info.size > 0);
    memcpy(alloc_info.memory_ptr, data, alloc_info.size * sizeof(uint32_t));
    vb->is_dirty = true;
}

void rpe_vertex_buffer_copy_index_data_u16(
    rpe_vertex_buffer_t* vb, rpe_vertex_alloc_info_t alloc_info, const int16_t* data)
{
    assert(data);
    assert(alloc_info.memory_ptr);
    assert(alloc_info.size > 0);

    // Currently we convert 16-bit integers to 32-bit.
    // FIXME: Add 16bit support to Vulkan backend.
    uint32_t* ptr = (uint32_t*)alloc_info.memory_ptr;
    for (size_t i = 0; i < alloc_info.size; ++i, ptr++)
    {
        int32_t val = (int32_t)data[i];
        memcpy(ptr, &val, sizeof(int32_t));
    }

    vb->is_dirty = true;
}

void rpe_vertex_buffer_upload_to_gpu(rpe_vertex_buffer_t* vb, vkapi_driver_t* driver)
{
    assert(vb);
    if (!vb->is_dirty)
    {
        return;
    }

    vkapi_driver_map_gpu_vertex(
        driver,
        vb->vertex_buffer,
        vb->vertex_data,
        vb->curr_vertex_size * sizeof(rpe_vertex_t),
        vb->index_buffer,
        vb->index_data,
        vb->curr_index_size * sizeof(uint32_t));
    vb->is_dirty = false;
}