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

#include "vertex_buffer.h"

#include <utility/arena.h>
#include <vulkan-api/buffer.h>
#include <vulkan-api/driver.h>

rpe_vertex_buffer_t* rpe_vertex_buffer_init(vkapi_driver_t* driver, arena_t* arena)
{
    rpe_vertex_buffer_t* i = ARENA_MAKE_ZERO_STRUCT(arena, rpe_vertex_buffer_t);

    i->vertex_data =
        ARENA_MAKE_ARRAY(arena, rpe_vertex_t, RPE_VERTEX_GPU_BUFFER_SIZE, ARENA_ZERO_MEMORY);
    i->index_data = ARENA_MAKE_ARRAY(arena, uint32_t, RPE_INDEX_GPU_BUFFER_SIZE, ARENA_ZERO_MEMORY);

    i->vertex_buffer =
        vkapi_res_cache_create_vertex_buffer(driver->res_cache, driver, RPE_VERTEX_GPU_BUFFER_SIZE);
    i->index_buffer =
        vkapi_res_cache_create_index_buffer(driver->res_cache, driver, RPE_INDEX_GPU_BUFFER_SIZE);

    return i;
}

uint32_t rpe_vertex_buffer_copy_vert_data(rpe_vertex_buffer_t* vb, size_t size, rpe_vertex_t* data)
{
    assert(vb);
    assert(data);
    assert(vb->curr_vertex_size + size < VKAPI_RES_CACHE_MAX_VERTEX_BUFFER_SIZE);
    rpe_vertex_t* ptr = vb->vertex_data + vb->curr_vertex_size;
    memcpy(ptr, data, size * sizeof(rpe_vertex_t));
    uint32_t out = vb->curr_vertex_size;
    vb->curr_vertex_size += size;
    vb->is_dirty = true;
    return out;
}

uint32_t rpe_vertex_buffer_ccpy_index_data(rpe_vertex_buffer_t* vb, size_t size, int32_t* data)
{
    assert(vb);
    assert(data);
    assert(vb->curr_index_size + size < VKAPI_RES_CACHE_MAX_INDEX_BUFFER_SIZE);
    uint32_t* ptr = vb->index_data + vb->curr_index_size;
    memcpy(ptr, data, size * sizeof(uint32_t));
    uint32_t out = vb->curr_index_size;
    vb->curr_index_size += size;
    vb->is_dirty = true;
    return out;
}

void rpe_vertex_buffer_upload_to_gpu(rpe_vertex_buffer_t* vb, vkapi_driver_t* driver)
{
    assert(vb);
    if (!vb->is_dirty)
    {
        return;
    }

    vkapi_buffer_t* vert_buffer = vkapi_res_cache_get_buffer(driver->res_cache, vb->vertex_buffer);
    vkapi_buffer_t* index_buffer = vkapi_res_cache_get_buffer(driver->res_cache, vb->index_buffer);

    vkapi_buffer_upload_vertex_data(
        vert_buffer, driver, vb->vertex_data, vb->curr_vertex_size * sizeof(rpe_vertex_t), 0);
    vkapi_buffer_upload_index_data(
        index_buffer, driver, vb->index_data, vb->curr_index_size * sizeof(uint32_t), 0);
    vb->is_dirty = false;
}