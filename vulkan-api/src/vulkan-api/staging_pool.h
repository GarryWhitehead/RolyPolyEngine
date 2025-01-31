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

#ifndef __VKAPI_STAGING_POOL_H__
#define __VKAPI_STAGING_POOL_H__

#include "common.h"

#include <utility/arena.h>

// Forward declarations.
typedef struct VkApiContext vkapi_context_t;

typedef struct VkApiStageInstance
{
    VkBuffer buffer;
    VkDeviceSize size;
    VmaAllocation mem;
    VmaAllocationInfo alloc_info;
    uint64_t frame_last_used;
} vkapi_staging_instance_t;

/**
 A simplistic staging pool for CPU-only stages. Used when copying to and from
 GPU only memory.
 */
typedef struct VkApiStagingPool
{
    // a list of free stages and their size
    arena_dyn_array_t free_stages;
    arena_dyn_array_t in_use_stages;
    uint64_t current_frame;

} vkapi_staging_pool_t;

vkapi_staging_pool_t* vkapi_staging_init(arena_t* perm_arena);

vkapi_staging_instance_t* vkapi_staging_get(
    vkapi_staging_pool_t* staging_pool, VmaAllocator vma_alloc, VkDeviceSize req_size);

void vkapi_staging_gc(
    vkapi_staging_pool_t* staging_pool, VmaAllocator vma_alloc, uint64_t current_frame);

void vkapi_staging_destroy(vkapi_staging_pool_t* staging_pool, VmaAllocator vma_alloc);

#endif