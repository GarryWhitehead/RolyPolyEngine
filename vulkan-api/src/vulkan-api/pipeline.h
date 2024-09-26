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

#pragma once

#include "backend/enums.h"
#include "common.h"
#include "pipeline_cache.h"

#include <stdint.h>
#include <utility/hash_set.h>

#define VKAPI_PIPELINE_LIFETIME_FRAME_COUNT 10
#define VKAPI_PIPELINE_MAX_DESC_SET_LAYOUT_BINDING_COUNT 10

// Forward declarations.
typedef struct VkApiContext vkapi_context_t;
typedef struct PipelineLayout vkapi_pl_layout_t;
typedef struct GraphicsPipeline vkapi_graphics_pl_t;
typedef struct ComputePipeline vkapi_compute_pl_t;

typedef struct PushBlockBindParams
{
    VkShaderStageFlags stage;
    uint32_t size;
    void* data;
} push_block_bind_params_t;

vkapi_pl_layout_t* vkapi_pl_layout_init(arena_t* arena);

void vkapi_pl_layout_add_desc_layout(
    vkapi_pl_layout_t* layout,
    uint32_t set,
    uint32_t binding,
    VkDescriptorType desc_type,
    VkShaderStageFlags stage,
    arena_t* arena);

/* Pipeline layout functions */

void vkapi_pl_layout_add_push_const(vkapi_pl_layout_t* layout, enum ShaderStage stage, size_t size);

void vkapi_pl_layout_bind_push_block(
    vkapi_pl_layout_t* layout, VkCommandBuffer cmdBuffer, push_block_bind_params_t* push_block);

void vkapi_pl_clear_descs(vkapi_pl_layout_t* layout);

/* Graphics pipeline functions */

/**
 Create a Vulkan graphics pipeline.
 @param context A Vulkan context.
 @param [out] pipeline The newly create pipeline instamce. NULL if fails.
 @param layout A pipeline layout associated with this graphics pipeline.
 @param key A pipeline cache key, used for caching the pipeline in the map.
 */
void vkapi_graph_pl_create(
    vkapi_context_t* context,
    vkapi_graphics_pl_t* pipeline,
    vkapi_pl_layout_t* layout,
    graphics_pl_key_t* key);

/* Compute pipeline functions */

/**
 Create a Vulkan compute pipeline.
 @param [out] pipeline The new compute pipeline instance. NULL if initialisation fails.
 @param context A Vulkan context.
 @param key A pipeline cache key.
 @param layout A pipeline layout associated with this compute pipeline.
 */
void vkapi_compute_pl_create(
    vkapi_compute_pl_t* pipeline,
    vkapi_context_t* context,
    compute_pl_key_t* key,
    vkapi_pl_layout_t* layout);