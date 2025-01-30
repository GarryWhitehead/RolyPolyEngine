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

#include <stdint.h>
#include <utility/hash_set.h>

#define VKAPI_PIPELINE_LIFETIME_FRAME_COUNT 10
#define VKAPI_PIPELINE_MAX_DESC_SET_LAYOUT_BINDING_COUNT 10
#define VKAPI_PIPELINE_MAX_SPECIALIZATION_COUNT 20
#define VKAPI_PIPELINE_MAX_VERTEX_ATTR_COUNT 10
#define VKAPI_PIPELINE_MAX_INPUT_BIND_COUNT 4

// Forward declarations.
typedef struct VkApiContext vkapi_context_t;
typedef struct PipelineLayout vkapi_pl_layout_t;
typedef struct GraphicsPipeline vkapi_graphics_pl_t;
typedef struct ComputePipeline vkapi_compute_pl_t;
typedef struct GraphicsPipelineKey graphics_pl_key_t;
typedef struct ComputePipelineKey compute_pl_key_t;
struct SpecConstParams;

typedef struct GraphicsPipeline
{
    // dynamic states to be used with this pipeline - by default the viewport
    // and scissor dynamic states are set
    VkDynamicState dyn_states[6];
    uint32_t dyn_state_count;

    VkPipeline instance;
    uint64_t last_used_frame_stamp;
} vkapi_graphics_pl_t;

typedef struct ComputePipeline
{
    VkPipeline instance;

} vkapi_compute_pl_t;

/**
 Create a Vulkan graphics pipeline.
 @param context A Vulkan context.
 @param layout A pipeline layout associated with this graphics pipeline.
 @param key A pipeline cache key, used for caching the pipeline in the map.
 */
vkapi_graphics_pl_t vkapi_graph_pl_create(
    vkapi_context_t* context, const graphics_pl_key_t* key, struct SpecConstParams* spec_consts);

/**
 Create a Vulkan compute pipeline.
 @param [out] pipeline The new compute pipeline instance. NULL if initialisation fails.
 @param context A Vulkan context.
 @param key A pipeline cache key.
 @param layout A pipeline layout associated with this compute pipeline.
 */
vkapi_compute_pl_t vkapi_compute_pl_create(vkapi_context_t* context, compute_pl_key_t* key);