#include "staging_pool.h"

#include "driver.h"

#include <assert.h>
#include <utility/array_utility.h>

vkapi_staging_instance_t _vkapi_staging_create(VmaAllocator vma_alloc, VkDeviceSize size)
{
    assert(size > 0);

    vkapi_staging_instance_t instance = {0};
    instance.size = size;
    instance.frame_last_used = 0;

    VkBufferCreateInfo bufferInfo = {0};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.size = size;

    // cpu staging pool
    VmaAllocationCreateInfo create_info = {0};
    create_info.usage = VMA_MEMORY_USAGE_AUTO;
    create_info.flags =
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    VMA_CHECK_RESULT(vmaCreateBuffer(
        vma_alloc,
        &bufferInfo,
        &create_info,
        &instance.buffer,
        &instance.mem,
        &instance.alloc_info));

    return instance;
}

vkapi_staging_pool_t* vkapi_staging_init(arena_t* perm_arena)
{
    vkapi_staging_pool_t* instance = ARENA_MAKE_ZERO_STRUCT(perm_arena, vkapi_staging_pool_t);
    MAKE_DYN_ARRAY(vkapi_staging_instance_t, perm_arena, 50, &instance->stages);
    MAKE_DYN_ARRAY(uint32_t, perm_arena, 50, &instance->in_use_stages);
    return instance;
}

vkapi_staging_instance_t*
vkapi_staging_get(vkapi_staging_pool_t* staging_pool, VmaAllocator vma_alloc, VkDeviceSize req_size)
{
    // Check for a free staging space that is equal or greater than the required
    // size.
    uint32_t stage_idx = UINT32_MAX;
    for (uint32_t i = 0; i < staging_pool->stages.size; ++i)
    {
        vkapi_staging_instance_t instance =
            DYN_ARRAY_GET(vkapi_staging_instance_t, &staging_pool->stages, i);
        if (instance.size >= req_size)
        {
            stage_idx = i;
            break;
        }
    }

    // if we have a free staging area, return that. Otherwise, allocate a new
    // stage.
    if (stage_idx != UINT32_MAX)
    {
        DYN_ARRAY_APPEND(&staging_pool->in_use_stages, &stage_idx);
        return &DYN_ARRAY_GET(vkapi_staging_instance_t, &staging_pool->stages, stage_idx);
    }

    vkapi_staging_instance_t new_instance = _vkapi_staging_create(vma_alloc, req_size);
    DYN_ARRAY_APPEND(&staging_pool->stages, &new_instance);
    return DYN_ARRAY_APPEND(&staging_pool->in_use_stages, &staging_pool->stages.size - 1);
}

void vkapi_staging_gc(
    vkapi_staging_pool_t* staging_pool, VmaAllocator vma_alloc, uint64_t current_frame)
{
    // destroy buffers that have not been used in some time
    for (uint32_t i = 0; i < staging_pool->stages.size; ++i)
    {
        vkapi_staging_instance_t* stage =
            DYN_ARRAY_GET_PTR(vkapi_staging_instance_t, &staging_pool->stages, i);
        uint64_t collect_frame = stage->frame_last_used + VKAPI_MAX_COMMAND_BUFFER_SIZE;
        if (collect_frame < current_frame)
        {
            // buffers that were currently in use can be moved to the
            // free stage container once it is safe to do so.
            uint32_t idx = ARRAY_UTIL_FIND(
                uint32_t, &i, staging_pool->in_use_stages.size, staging_pool->in_use_stages.data);
            if (idx != UINT32_MAX)
            {
                DYN_ARRAY_REMOVE(&staging_pool->in_use_stages, idx);
                continue;
            }
            vmaDestroyBuffer(vma_alloc, stage->buffer, stage->mem);
            DYN_ARRAY_REMOVE(&staging_pool->stages, i);
        }
    }
}

void vkapi_staging_destroy(vkapi_staging_pool_t* staging_pool, VmaAllocator vma_alloc)
{
    for (uint32_t i = 0; i < staging_pool->stages.size; ++i)
    {
        vkapi_staging_instance_t instance =
            DYN_ARRAY_GET(vkapi_staging_instance_t, &staging_pool->stages, i);
        vmaDestroyBuffer(vma_alloc, instance.buffer, instance.mem);
    }

    dyn_array_clear(&staging_pool->stages);
    dyn_array_clear(&staging_pool->in_use_stages);
}
