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

#include "descriptor_cache.h"

#include "driver.h"
#include "pipeline_cache.h"
#include "program_manager.h"
#include "shader.h"
#include "texture.h"

#include <utility/hash.h>

vkapi_desc_cache_t* vkapi_desc_cache_init(vkapi_driver_t* driver, arena_t* arena)
{
    vkapi_desc_cache_t* c = ARENA_MAKE_ZERO_STRUCT(arena, vkapi_desc_cache_t);
    c->driver = driver;
    c->current_desc_pool_size = 1000;

    vkapi_desc_cache_create_pool(c);
    c->descriptor_sets = HASH_SET_CREATE(desc_key_t, vkapi_desc_set_t, arena);
    MAKE_DYN_ARRAY(vkapi_desc_set_t, arena, 100, &c->desc_sets_for_deletion);
    MAKE_DYN_ARRAY(VkDescriptorPool, arena, 100, &c->desc_pools_for_deletion);

    return c;
}

void vkapi_desc_cache_reset_keys(vkapi_desc_cache_t* c)
{
    memset(&c->desc_requires, 0, sizeof(desc_key_t));
}

bool vkapi_desc_cache_compare_desc_keys(desc_key_t* lhs, desc_key_t* rhs)
{
    return memcmp(lhs, rhs, sizeof(desc_key_t)) == 0;
}

void vkapi_desc_cache_bind_descriptors(
    vkapi_desc_cache_t* c,
    VkCommandBuffer cmd_buffer,
    shader_prog_bundle_t* bundle,
    VkPipelineLayout layout,
    VkPipelineBindPoint bind_point,
    bool force_rebind)
{
    // Check if the required descriptor set is already bound. If so, nothing to
    // do here.
    if (vkapi_desc_cache_compare_desc_keys(&c->desc_requires, &c->bound_desc) && !force_rebind)
    {
        vkapi_desc_set_t* s = HASH_SET_GET(&c->descriptor_sets, &c->bound_desc);
        s->frame_last_used = c->driver->current_frame;
        vkapi_desc_cache_reset_keys(c);
        return;
    }

    vkapi_desc_set_t* s = HASH_SET_GET(&c->descriptor_sets, &c->desc_requires);
    if (s)
    {
        s->frame_last_used = c->driver->current_frame;
    }
    else
    {
        // Create a new descriptor set if no cached set matches the requirements.
        vkapi_desc_set_t new_set = vkapi_desc_cache_create_desc_sets(c, bundle);
        new_set.frame_last_used = c->driver->current_frame;
        s = HASH_SET_INSERT(&c->descriptor_sets, &c->desc_requires, &new_set);
    }

    vkCmdBindDescriptorSets(
        cmd_buffer,
        bind_point,
        layout,
        0,
        VKAPI_PIPELINE_MAX_DESC_SET_COUNT,
        s->desc_sets,
        0,
        VK_NULL_HANDLE);

    c->bound_desc = c->desc_requires;
    vkapi_desc_cache_reset_keys(c);
}

void vkapi_desc_cache_bind_sampler(vkapi_desc_cache_t* c, struct DescriptorImage* images)
{
    assert(c);
    memcpy(
        c->desc_requires.samplers,
        images,
        sizeof(struct DescriptorImage) * VKAPI_PIPELINE_MAX_SAMPLER_BIND_COUNT);
}

void vkapi_desc_cache_bind_storage_image(vkapi_desc_cache_t* c, struct DescriptorImage* images)
{
    assert(c);
    memcpy(
        c->desc_requires.storage_images,
        images,
        sizeof(struct DescriptorImage) * VKAPI_PIPELINE_MAX_STORAGE_IMAGE_BOUND_COUNT);
}

void vkapi_desc_cache_bind_ubo(
    vkapi_desc_cache_t* c, uint8_t bind_value, VkBuffer buffer, uint32_t size)
{
    assert(c);
    assert(bind_value < VKAPI_PIPELINE_MAX_UBO_BIND_COUNT);
    c->desc_requires.ubos[bind_value] = buffer;
    c->desc_requires.buffer_sizes[bind_value] = size;
}

void vkapi_desc_cache_bind_ubo_dynamic(
    vkapi_desc_cache_t* c, uint8_t bind_value, VkBuffer buffer, uint32_t size)
{
    assert(c);
    assert(bind_value < VKAPI_PIPELINE_MAX_DYNAMIC_UBO_BIND_COUNT);
    assert(size > 0);
    c->desc_requires.dynamic_ubos[bind_value] = buffer;
    c->desc_requires.dynamic_buffer_sizes[bind_value] = size;
}

void vkapi_desc_cache_bind_ssbo(
    vkapi_desc_cache_t* c, uint8_t bind_value, VkBuffer buffer, uint32_t size)
{
    assert(c);
    assert(size > 0);
    assert(bind_value < VKAPI_PIPELINE_MAX_SSBO_BIND_COUNT);
    c->desc_requires.ssbos[bind_value] = buffer;
    c->desc_requires.ssbo_buffer_sizes[bind_value] = size;
}

vkapi_desc_set_t
vkapi_desc_cache_create_desc_sets(vkapi_desc_cache_t* c, shader_prog_bundle_t* bundle)
{
    assert(c);
    assert(bundle);

    vkapi_desc_set_t ds = {0};
    memcpy(
        ds.layout,
        bundle->desc_layouts,
        sizeof(VkDescriptorSetLayout) * VKAPI_PIPELINE_MAX_DESC_SET_COUNT);

    if (c->descriptor_sets.size * VKAPI_PIPELINE_MAX_DESC_SET_COUNT > c->current_desc_pool_size)
    {
        vkapi_desc_cache_increase_pool_capacity(c);
    }

    // Needed for the bindless samplers - we specify the number of samplers now which will
    // be all those held by the resource cache.
    VkDescriptorSetVariableDescriptorCountAllocateInfoEXT ext_info = {0};
    uint32_t variable_count = VKAPI_PIPELINE_MAX_SAMPLER_BINDLESS_COUNT;

    // Create a descriptor set for each layout.
    VkDescriptorSetAllocateInfo ai = {0};
    ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ai.descriptorPool = c->descriptor_pool;

    for (int i = 0; i < VKAPI_PIPELINE_MAX_DESC_SET_COUNT; ++i)
    {
        ai.pNext = VK_NULL_HANDLE;
        ai.pSetLayouts = &ds.layout[i];
        ai.descriptorSetCount = 1;
        if (i == VKAPI_PIPELINE_SAMPLER_SET_VALUE && !bundle->use_bound_samplers &&
            bundle->desc_binding_counts[VKAPI_PIPELINE_SAMPLER_SET_VALUE] > 0)
        {
            ext_info.sType =
                VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
            ext_info.descriptorSetCount = 1;
            ext_info.pDescriptorCounts = &variable_count;
            ai.pNext = &ext_info;
        }
        VK_CHECK_RESULT(
            vkAllocateDescriptorSets(c->driver->context->device, &ai, &ds.desc_sets[i]));
    }

    // Update the descriptor sets for each type (buffer, sampler, attachment).
    VkWriteDescriptorSet write_sets[VKAPI_PIPELINE_MAX_DESC_SET_COUNT * 6] = {};
    size_t write_set_count = 0;

    VkDescriptorBufferInfo buffer_info[VKAPI_PIPELINE_MAX_UBO_BIND_COUNT] = {};
    VkDescriptorBufferInfo ssbo_info[VKAPI_PIPELINE_MAX_SSBO_BIND_COUNT] = {};
    VkDescriptorImageInfo sampler_info[VKAPI_PIPELINE_MAX_SAMPLER_BIND_COUNT] = {};
    VkDescriptorImageInfo storage_image_info[VKAPI_PIPELINE_MAX_STORAGE_IMAGE_BOUND_COUNT] = {};

    // ==================== buffer descriptor writes ===============
    // Uniform buffers.
    for (uint8_t bind = 0; bind < VKAPI_PIPELINE_MAX_UBO_BIND_COUNT; ++bind)
    {
        if (c->desc_requires.ubos[bind])
        {
            VkDescriptorBufferInfo* bi = &buffer_info[bind];
            bi->buffer = c->desc_requires.ubos[bind];
            bi->offset = 0;
            bi->range = c->desc_requires.buffer_sizes[bind];

            VkWriteDescriptorSet* ws = &write_sets[write_set_count++];
            ws->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            ws->dstSet = ds.desc_sets[VKAPI_PIPELINE_UBO_SET_VALUE];
            ws->pBufferInfo = bi;
            ws->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            ws->dstBinding = bind;
            ws->descriptorCount = 1;
        }
    }
    // storage buffers
    for (uint8_t bind = 0; bind < VKAPI_PIPELINE_MAX_SSBO_BIND_COUNT; ++bind)
    {
        if (c->desc_requires.ssbos[bind])
        {
            VkDescriptorBufferInfo* bi = &ssbo_info[bind];
            bi->buffer = c->desc_requires.ssbos[bind];
            bi->offset = 0;
            bi->range = c->desc_requires.ssbo_buffer_sizes[bind];

            VkWriteDescriptorSet* ws = &write_sets[write_set_count++];
            ws->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            ws->dstSet = ds.desc_sets[VKAPI_PIPELINE_SSBO_SET_VALUE];
            ws->pBufferInfo = bi;
            ws->descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            ws->dstBinding = bind;
            ws->descriptorCount = 1;
        }
    }

    //  =============== image descriptor writes =======================

    if (bundle->desc_binding_counts[VKAPI_PIPELINE_SAMPLER_SET_VALUE] > 0)
    {
        if (bundle->use_bound_samplers)
        {
            for (uint8_t bind = 0; bind < VKAPI_PIPELINE_MAX_SAMPLER_BIND_COUNT; ++bind)
            {
                if (c->desc_requires.samplers[bind].image_view)
                {
                    VkDescriptorImageInfo* ii = &sampler_info[bind];
                    ii->imageView = c->desc_requires.samplers[bind].image_view;
                    ii->imageLayout = c->desc_requires.samplers[bind].image_layout;
                    ii->sampler = c->desc_requires.samplers[bind].image_sampler;

                    VkWriteDescriptorSet* ws = &write_sets[write_set_count++];
                    ws->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    ws->dstSet = ds.desc_sets[VKAPI_PIPELINE_SAMPLER_SET_VALUE];
                    ws->pImageInfo = ii;
                    ws->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    ws->dstBinding = bind;
                    ws->descriptorCount = 1;
                }
            }
        }
        else
        {
            // As image samplers are bindless, all textures are bound which are
            // currently held by the resource cache.
            vkapi_res_cache_t* rs = c->driver->res_cache;
            uint32_t count = 0;
            VkDescriptorImageInfo* ii = ARENA_MAKE_ZERO_ARRAY(
                &c->driver->_scratch_arena, VkDescriptorImageInfo, rs->textures.size);

            assert(rs->textures.size >= VKAPI_RES_CACHE_MAX_RESERVED_COUNT);
            for (uint32_t i = VKAPI_RES_CACHE_MAX_RESERVED_COUNT; i < rs->textures.size; ++i)
            {
                vkapi_texture_t* tex = DYN_ARRAY_GET_PTR(vkapi_texture_t, &rs->textures, i);
                ii[count].imageView = tex->image_views[0];
                ii[count].imageLayout = tex->image_layout;
                ii[count++].sampler = tex->sampler;
                assert(tex->sampler && tex->image_views[0]);
            }
            if (count > 0)
            {
                VkWriteDescriptorSet* ws = &write_sets[write_set_count++];
                ws->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                ws->dstSet = ds.desc_sets[VKAPI_PIPELINE_SAMPLER_SET_VALUE];
                ws->pImageInfo = ii;
                ws->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                // There is a mandatory bind value of zero for bindless textures.
                ws->dstBinding = 0;
                ws->descriptorCount = count;
            }
        }
    }

    // Storage images.
    for (uint8_t bind = 0; bind < VKAPI_PIPELINE_MAX_STORAGE_IMAGE_BOUND_COUNT; ++bind)
    {
        if (c->desc_requires.storage_images[bind].image_view)
        {
            VkDescriptorImageInfo* ii = &storage_image_info[bind];
            ii->imageView = c->desc_requires.storage_images[bind].image_view;
            ii->imageLayout = c->desc_requires.storage_images[bind].image_layout;
            ii->sampler = c->desc_requires.storage_images[bind].image_sampler;

            VkWriteDescriptorSet* ws = &write_sets[write_set_count++];
            ws->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            ws->dstSet = ds.desc_sets[VKAPI_PIPELINE_STORAGE_IMAGE_SET_VALUE];
            ws->pImageInfo = ii;
            ws->descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            ws->dstBinding = bind;
            ws->descriptorCount = 1;
        }
    }

    vkUpdateDescriptorSets(
        c->driver->context->device, write_set_count, write_sets, 0, VK_NULL_HANDLE);

    arena_reset(&c->driver->_scratch_arena);
    return ds;
}

void vkapi_desc_cache_create_pool(vkapi_desc_cache_t* c)
{
    VkDescriptorPoolSize pools[VKAPI_PIPELINE_MAX_DESC_SET_COUNT];

    pools[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pools[0].descriptorCount = c->current_desc_pool_size * VKAPI_PIPELINE_MAX_UBO_BIND_COUNT;
    pools[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    pools[1].descriptorCount =
        c->current_desc_pool_size * VKAPI_PIPELINE_MAX_DYNAMIC_UBO_BIND_COUNT;
    // We over allocate by quite a margin if using bond samplers.
    pools[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pools[2].descriptorCount = VKAPI_PIPELINE_MAX_SAMPLER_BINDLESS_COUNT;
    pools[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pools[3].descriptorCount = c->current_desc_pool_size * VKAPI_PIPELINE_MAX_SSBO_BIND_COUNT;
    pools[4].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    pools[4].descriptorCount =
        c->current_desc_pool_size * VKAPI_PIPELINE_MAX_STORAGE_IMAGE_BOUND_COUNT;

    VkDescriptorPoolCreateInfo ci = {0};
    ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ci.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT |
        VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    ci.maxSets = c->current_desc_pool_size * VKAPI_PIPELINE_MAX_DESC_SET_COUNT;
    ci.poolSizeCount = VKAPI_PIPELINE_MAX_DESC_SET_COUNT;
    ci.pPoolSizes = pools;
    VK_CHECK_RESULT(vkCreateDescriptorPool(
        c->driver->context->device, &ci, VK_NULL_HANDLE, &c->descriptor_pool))
}

void vkapi_desc_cache_increase_pool_capacity(vkapi_desc_cache_t* c)
{
    assert(c);
    DYN_ARRAY_APPEND(&c->desc_pools_for_deletion, &c->descriptor_pool);

    // Schedule all descriptor sets associated with this pool for deletion as well.
    hash_set_iterator_t it = hash_set_iter_create(&c->descriptor_sets);
    for (;;)
    {
        vkapi_desc_set_t* set = hash_set_iter_next(&it);
        if (!set)
        {
            break;
        }
        DYN_ARRAY_APPEND(&c->desc_sets_for_deletion, set);
    }
    hash_set_clear(&c->descriptor_sets);

    c->current_desc_pool_size *= 2;
    vkapi_desc_cache_create_pool(c);
}

void vkapi_desc_cache_create_pl_layouts(vkapi_driver_t* driver, shader_prog_bundle_t* bundle)
{
    assert(bundle);

    for (uint32_t set_idx = 0; set_idx < VKAPI_PIPELINE_MAX_DESC_SET_COUNT; ++set_idx)
    {
        VkDescriptorSetLayoutBindingFlagsCreateInfoEXT ext_flags = {0};
        VkDescriptorBindingFlags bind_flags[VKAPI_PIPELINE_MAX_DESC_SET_COUNT] = {
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
            VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT |
            VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT};
        VkDescriptorSetLayoutCreateInfo layout_info = {0};

        if (set_idx == VKAPI_PIPELINE_SAMPLER_SET_VALUE && !bundle->use_bound_samplers &&
            bundle->desc_binding_counts[set_idx] > 0)
        {
            // Only samplers are bindless (so far). Use the variable descriptor count flag to allow
            // for an unsized sampler array.
            ext_flags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
            ext_flags.bindingCount = bundle->desc_binding_counts[set_idx];
            ext_flags.pBindingFlags = bind_flags;
            layout_info.pNext = &ext_flags;
            layout_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT;
        }

        VkDescriptorSetLayoutBinding* set_binding = bundle->desc_bindings[set_idx];
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = bundle->desc_binding_counts[set_idx];
        layout_info.pBindings = !layout_info.bindingCount ? VK_NULL_HANDLE : set_binding;
        vkCreateDescriptorSetLayout(
            driver->context->device, &layout_info, VK_NULL_HANDLE, &bundle->desc_layouts[set_idx]);
    }
}

void vkapi_desc_cache_gc(vkapi_desc_cache_t* c, uint64_t current_frame)
{
    assert(c);

    // Destroy any descriptor sets that have reached there lifetime after their last
    // use.
    // TODO: we really should be deleting the pipeline layout associated with
    // these sets too.
    hash_set_iterator_t it = hash_set_iter_create(&c->descriptor_sets);
    for (;;)
    {
        vkapi_desc_set_t* set = hash_set_iter_next(&it);
        if (!set)
        {
            break;
        }

        uint64_t collection_frame = set->frame_last_used + VKAPI_PIPELINE_LIFETIME_FRAME_COUNT;
        if (collection_frame < current_frame)
        {
            for (int i = 0; i < VKAPI_PIPELINE_MAX_DESC_SET_COUNT; ++i)
            {
                if (set->layout[i])
                {
                    // TODO: we cant destroy the set layouts as they are used
                    // by pipelinelayouts and doing so results in a crash
                    // So need to add a ref to the pipeline layout when creating
                    // the desc sets and delete everything.
                    // context_.device().destroy(info.layout[i]);
                }
            }
            vkFreeDescriptorSets(
                c->driver->context->device,
                c->descriptor_pool,
                VKAPI_PIPELINE_MAX_DESC_SET_COUNT,
                set->desc_sets);
            it = hash_set_iter_erase(&it);
        }
    }

    // remove stale sets and pools
    if (c->desc_sets_for_deletion.size)
    {
        vkapi_desc_set_t* set = DYN_ARRAY_GET_PTR(vkapi_desc_set_t, &c->desc_sets_for_deletion, 0);
        uint64_t collection_frame = set->frame_last_used + VKAPI_PIPELINE_LIFETIME_FRAME_COUNT;
        if (collection_frame < current_frame)
        {
            for (size_t i = 0; i < c->desc_pools_for_deletion.size; ++i)
            {
                VkDescriptorPool pool =
                    DYN_ARRAY_GET(VkDescriptorPool, &c->desc_pools_for_deletion, i);
                vkDestroyDescriptorPool(c->driver->context->device, pool, VK_NULL_HANDLE);
            }
        }
    }
}

void vkapi_desc_cache_destroy(vkapi_desc_cache_t* c)
{
    assert(c);

    // Destroy all descriptor sets and layouts associated with this cache.
    hash_set_iterator_t it = hash_set_iter_create(&c->descriptor_sets);
    for (;;)
    {
        vkapi_desc_set_t* set = hash_set_iter_next(&it);
        if (!set)
        {
            break;
        }
        vkFreeDescriptorSets(
            c->driver->context->device,
            c->descriptor_pool,
            VKAPI_PIPELINE_MAX_DESC_SET_COUNT,
            set->desc_sets);
    }
    hash_set_clear(&c->descriptor_sets);

    // Destroy the descriptor pool.
    vkDestroyDescriptorPool(c->driver->context->device, c->descriptor_pool, VK_NULL_HANDLE);
}
