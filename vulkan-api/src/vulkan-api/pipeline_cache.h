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

#include "backend/enums.h"
#include "common.h"

#ifndef __VKAPI_PIPELINE_CACHE_H__
#define __VKAPI_PIPELINE_CACHE_H__

#define VKAPI_PIPELINE_MAX_SAMPLER_BIND_COUNT 10
#define VKAPI_PIPELINE_MAX_UBO_BIND_COUNT 8
#define VKAPI_PIPELINE_MAX_DYNAMIC_UBO_BIND_COUNT 4
#define VKAPI_PIPELINE_MAX_SSBO_BIND_COUNT 4
#define VKAPI_PIPELINE_MAX_VERTEX_ATTR_COUNT 8
#define VKAPI_PIPELINE_MAX_STORAGE_IMAGE_BOUND_COUNT 6

// Shader set values for each descriptor type.
#define VKAPI_PIPELINE_UBO_SET_VALUE 0
#define VKAPI_PIPELINE_UBO_DYN_SET_VALUE 1
#define VKAPI_PIPELINE_SSBO_SET_VALUE 2
#define VKAPI_PIPELINE_SAMPLER_SET_VALUE 3
#define VKAPI_PIPELINE_STORAGE_IMAGE_SET_VALUE 4
#define VKAPI_PIPELINE_MAX_DESC_SET_COUNT 5

#define VKAPI_PIPELINE_MAX_PUSH_CONSTANT_COUNT 10


#pragma clang diagnostic push
#pragma clang diagnostic warning "-Wpadded"

struct RasterStateBlock
{
    VkCullModeFlagBits cull_mode;
    VkPolygonMode polygon_mode;
    VkFrontFace front_face;
    VkPrimitiveTopology topology;
    VkColorComponentFlags color_write_mask;
    uint32_t colour_attach_count;
    VkBool32 prim_restart;
    VkBool32 depth_test_enable;
    VkBool32 depth_write_enable;
};

struct DepthStencilBlock
{
    VkCompareOp compare_op;
    VkStencilOp stencil_fail_op;
    VkStencilOp depth_fail_op;
    VkStencilOp pass_op;
    uint32_t compare_mask;
    uint32_t write_mask;
    uint32_t reference;
    VkBool32 stencil_test_enable;
};

struct BlendFactorBlock
{
    VkBool32 blend_enable;
    VkBlendFactor src_color_blend_factor;
    VkBlendFactor dst_color_blend_factor;
    VkBlendOp color_blend_op;
    VkBlendFactor src_alpha_blend_factor;
    VkBlendFactor dst_alpha_blend_factor;
    VkBlendOp alpha_blend_op;
};

typedef struct GraphicsPipelineKey
{
    struct RasterStateBlock raster_state;
    struct DepthStencilBlock ds_block;
    struct BlendFactorBlock blend_state;
    VkRenderPass render_pass;
    VkPipelineShaderStageCreateInfo shaders[RPE_BACKEND_SHADER_STAGE_MAX_COUNT];
    VkVertexInputAttributeDescription vert_attr_descs[VKAPI_PIPELINE_MAX_VERTEX_ATTR_COUNT];
    VkVertexInputBindingDescription vert_bind_descs[VKAPI_PIPELINE_MAX_VERTEX_ATTR_COUNT];
    size_t tesse_vert_count;
} graphics_pl_key_t;

typedef struct ComputePipelineKey
{
    VkPipelineShaderStageCreateInfo shader;
} compute_pl_key_t;

#pragma clang diagnostic pop

#endif