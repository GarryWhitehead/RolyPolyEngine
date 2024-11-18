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

#ifndef __VKAPI_DESCRIPTOR_CACHE_H__
#define __VKAPI_DESCRIPTOR_CACHE_H__

#define VKAPI_PIPELINE_MAX_STORAGE_IMAGE_BOUND_COUNT 6
#define VKAPI_PIPELINE_MAX_PUSH_CONSTANT_COUNT 10
// Bindless samplers used by graphics pipeline.
#define VKAPI_PIPELINE_MAX_SAMPLER_BINDLESS_COUNT 1024
// Bound samplers for compute shaders.
#define VKAPI_PIPELINE_MAX_SAMPLER_BIND_COUNT 6
#define VKAPI_PIPELINE_MAX_UBO_BIND_COUNT 8
#define VKAPI_PIPELINE_MAX_DYNAMIC_UBO_BIND_COUNT 4
#define VKAPI_PIPELINE_MAX_SSBO_BIND_COUNT 4

// Shader set values for each descriptor type.
#define VKAPI_PIPELINE_UBO_SET_VALUE 0
#define VKAPI_PIPELINE_UBO_DYN_SET_VALUE 1
#define VKAPI_PIPELINE_SSBO_SET_VALUE 2
#define VKAPI_PIPELINE_SAMPLER_SET_VALUE 3
#define VKAPI_PIPELINE_STORAGE_IMAGE_SET_VALUE 4
#define VKAPI_PIPELINE_MAX_DESC_SET_COUNT 5

#include <vulkan-api/common.h>
#include <utility/hash_set.h>

typedef struct VkApiDriver vkapi_driver_t;
typedef struct ShaderProgramBundle shader_prog_bundle_t;
typedef struct PipelineLayout vkapi_pl_layout_t;
typedef struct Shader shader_t;

struct DescriptorImage
{
    VkImageView image_view;
    VkImageLayout image_layout;
    uint32_t padding;
    VkSampler image_sampler;
};

typedef struct DescriptorKey
{
    VkBuffer ubos[VKAPI_PIPELINE_MAX_UBO_BIND_COUNT];
    VkBuffer dynamic_ubos[VKAPI_PIPELINE_MAX_DYNAMIC_UBO_BIND_COUNT];
    VkBuffer ssbos[VKAPI_PIPELINE_MAX_SSBO_BIND_COUNT];
    size_t buffer_sizes[VKAPI_PIPELINE_MAX_UBO_BIND_COUNT];
    size_t dynamic_buffer_sizes[VKAPI_PIPELINE_MAX_DYNAMIC_UBO_BIND_COUNT];
    size_t ssbo_buffer_sizes[VKAPI_PIPELINE_MAX_SSBO_BIND_COUNT];
    struct DescriptorImage samplers[VKAPI_PIPELINE_MAX_SAMPLER_BIND_COUNT];
    struct DescriptorImage storage_images[VKAPI_PIPELINE_MAX_STORAGE_IMAGE_BOUND_COUNT];
} desc_key_t;

typedef struct DescriptorSetInfo
{
    VkDescriptorSetLayout layout[VKAPI_PIPELINE_MAX_DESC_SET_COUNT];
    VkDescriptorSet desc_sets[VKAPI_PIPELINE_MAX_DESC_SET_COUNT];
    uint64_t frame_last_used;
} vkapi_desc_set_t;

typedef struct DescriptorCache
{
    vkapi_driver_t* driver;
    hash_set_t descriptor_sets;
    desc_key_t desc_requires;
    /// current bound descriptor
    desc_key_t bound_desc;
    /// the main descriptor pool
    VkDescriptorPool descriptor_pool;
    uint32_t current_desc_pool_size;

    /// A pool of descriptor sets for each descriptor type.
    /// Reference to these sets are also stored in the cache - so
    /// the cached descriptor sets must be cleared if destroying the
    /// the sets stored in this pool.
    VkDescriptorSet desc_set_pool[VKAPI_PIPELINE_MAX_DESC_SET_COUNT];

    // containers for storing pools and sets that are
    // waiting to be destroyed once they reach their lifetime.
    arena_dyn_array_t desc_pools_for_deletion;
    arena_dyn_array_t desc_sets_for_deletion;
    // Set if samplers are explicitly bound - otherwise assumes bindless textures.
    bool use_bound_samplers;
} vkapi_desc_cache_t;

void vkapi_desc_cache_bind_descriptors(
    vkapi_desc_cache_t* c,
    VkCommandBuffer cmdBuffer,
    shader_prog_bundle_t* bundle,
    VkPipelineLayout layout,
    VkPipelineBindPoint bind_point);

vkapi_desc_cache_t* vkapi_desc_cache_init(vkapi_driver_t* driver, arena_t* arena);
void vkapi_desc_cache_reset_keys(vkapi_desc_cache_t* c);

void vkapi_desc_cache_bind_sampler(vkapi_desc_cache_t* c, struct DescriptorImage* images);
void vkapi_desc_cache_bind_storage_image(vkapi_desc_cache_t* c, struct DescriptorImage* images);
void vkapi_desc_cache_bind_ubo(
    vkapi_desc_cache_t* c, uint8_t bind_value, VkBuffer buffer, uint32_t size);
void vkapi_desc_cache_bind_ubo_dynamic(
    vkapi_desc_cache_t* c, uint8_t bind_value, VkBuffer buffer, uint32_t size);
void vkapi_desc_cache_bind_ssbo(
    vkapi_desc_cache_t* c, uint8_t bind_value, VkBuffer buffer, uint32_t size);

vkapi_desc_set_t vkapi_desc_cache_create_desc_sets(
    vkapi_desc_cache_t* c, shader_prog_bundle_t* bundle);

void vkapi_desc_cache_create_pool(vkapi_desc_cache_t* c);

void vkapi_desc_cache_increase_pool_capacity(vkapi_desc_cache_t* c);

void vkapi_desc_cache_gc(vkapi_desc_cache_t* c, uint64_t current_frame);
void vkapi_desc_cache_destroy(vkapi_desc_cache_t* c);

void vkapi_desc_cache_create_layouts(
    shader_t* shader, vkapi_driver_t* driver, shader_prog_bundle_t* bundle, arena_t* arena);

#endif