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
    create_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;
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
    MAKE_DYN_ARRAY(vkapi_staging_instance_t, perm_arena, 50, &instance->free_stages);
    MAKE_DYN_ARRAY(vkapi_staging_instance_t, perm_arena, 50, &instance->in_use_stages);
    return instance;
}

vkapi_staging_instance_t*
vkapi_staging_get(vkapi_staging_pool_t* staging_pool, VmaAllocator vma_alloc, VkDeviceSize req_size)
{
    assert(staging_pool);
    assert(req_size > 0);

    // Check for a free staging space that is equal or greater than the required
    // size.
    for (uint32_t i = 0; i < staging_pool->free_stages.size; ++i)
    {
        vkapi_staging_instance_t instance =
            DYN_ARRAY_GET(vkapi_staging_instance_t, &staging_pool->free_stages, i);
        if (instance.size >= req_size)
        {
            DYN_ARRAY_REMOVE(&staging_pool->free_stages, i);
            instance.frame_last_used = staging_pool->current_frame;
            return DYN_ARRAY_APPEND(&staging_pool->in_use_stages, &instance);
        }
    }

    vkapi_staging_instance_t new_instance = _vkapi_staging_create(vma_alloc, req_size);
    new_instance.frame_last_used = staging_pool->current_frame;
    return DYN_ARRAY_APPEND(&staging_pool->in_use_stages, &new_instance);
}

void vkapi_staging_gc(
    vkapi_staging_pool_t* staging_pool, VmaAllocator vma_alloc, uint64_t current_frame)
{
    if (staging_pool->current_frame < VKAPI_MAX_COMMAND_BUFFER_SIZE)
    {
        ++current_frame;
        return;
    }

    for (uint32_t i = 0; i < staging_pool->free_stages.size; ++i)
    {
        vkapi_staging_instance_t* stage =
            DYN_ARRAY_GET_PTR(vkapi_staging_instance_t, &staging_pool->free_stages, i);
        uint64_t collect_frame = stage->frame_last_used + VKAPI_MAX_COMMAND_BUFFER_SIZE;
        if (collect_frame < current_frame)
        {
            vmaDestroyBuffer(vma_alloc, stage->buffer, stage->mem);
            DYN_ARRAY_REMOVE(&staging_pool->free_stages, i);
        }
    }

    // In use buffers that have definitely been executed on cmd bufefrs are moved to the free stage
    // container for re-use.
    for (uint32_t i = 0; i < staging_pool->in_use_stages.size; ++i)
    {
        vkapi_staging_instance_t* stage =
            DYN_ARRAY_GET_PTR(vkapi_staging_instance_t, &staging_pool->in_use_stages, i);
        uint64_t collect_frame = stage->frame_last_used + VKAPI_MAX_COMMAND_BUFFER_SIZE;
        if (collect_frame < current_frame)
        {
            DYN_ARRAY_APPEND(&staging_pool->free_stages, stage);
            DYN_ARRAY_REMOVE(&staging_pool->in_use_stages, i);
        }
    }
    ++current_frame;
}

void vkapi_staging_destroy(vkapi_staging_pool_t* staging_pool, VmaAllocator vma_alloc)
{
    for (uint32_t i = 0; i < staging_pool->free_stages.size; ++i)
    {
        vkapi_staging_instance_t instance =
            DYN_ARRAY_GET(vkapi_staging_instance_t, &staging_pool->free_stages, i);
        vmaDestroyBuffer(vma_alloc, instance.buffer, instance.mem);
    }

    for (uint32_t i = 0; i < staging_pool->in_use_stages.size; ++i)
    {
        vkapi_staging_instance_t instance =
            DYN_ARRAY_GET(vkapi_staging_instance_t, &staging_pool->in_use_stages, i);
        vmaDestroyBuffer(vma_alloc, instance.buffer, instance.mem);
    }

    dyn_array_clear(&staging_pool->free_stages);
    dyn_array_clear(&staging_pool->in_use_stages);
}
