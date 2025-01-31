/* Copyright (c) 2022 Garry Whitehead
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

#ifndef __BACKEND_BACKEND_ENUMS_H__
#define __BACKEND_BACKEND_ENUMS_H__

#include <stdint.h>
#include <utility/compiler.h>
#include <vulkan-api/common.h>

enum BlendFactor
{
    RPE_BLEND_FACTOR_ZERO,
    RPE_BLEND_FACTOR_ONE,
    RPE_BLEND_FACTOR_SRC_COL,
    RPE_BLEND_FACTOR_ONE_MINUS_SRC_COL,
    RPE_BLEND_FACTOR_DST_COL,
    RPE_BLEND_FACTOR_ONE_MINUS_DST_COL,
    RPE_BLEND_FACTOR_SRC_ALPHA,
    RPE_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    RPE_BLEND_FACTOR_DST_ALPHA,
    RPE_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
    RPE_BLEND_FACTOR_CONST_COL,
    RPE_BLEND_FACTOR_ONE_MINUS_CONST_COL,
    RPE_BLEND_FACTOR_CONST_ALPHA,
    RPE_BLEND_FACTOR_ONE_MINUS_CONST_ALPHA,
    RPE_BLEND_FACTOR_SRC_ALPHA_SATURATE
};

enum BlendOp
{
    RPE_BLEND_OP_ADD,
    RPE_BLEND_OP_SUB,
    RPE_BLEND_OP_REV_SUB,
    RPE_BLEND_OP_MIN,
    RPE_BLEND_OP_MAX,
};

enum BlendFactorPresets
{
    RPE_BLEND_FACTOR_PRESET_TRANSLUCENT
};

enum SamplerAddressMode
{
    RPE_SAMPLER_ADDR_MODE_REPEAT,
    RPE_SAMPLER_ADDR_MODE_MIRRORED_REPEAT,
    RPE_SAMPLER_ADDR_MODE_CLAMP_TO_EDGE,
    RPE_SAMPLER_ADDR_MODE_CLAMP_TO_BORDER,
    RPE_SAMPLER_ADDR_MODE_MIRROR_CLAMP_TO_EDGE
};

enum SamplerFilter
{
    RPE_SAMPLER_FILTER_NEAREST,
    RPE_SAMPLER_FILTER_LINEAR,
    RPE_SAMPLER_FILTER_CUBIC
};

enum CullMode
{
    RPE_CULL_MODE_BACK,
    RPE_CULL_MODE_FRONT,
    RPE_CULL_MODE_NONE
};

enum FrontFace
{
    RPE_FRONT_FACE_CLOCKWISE,
    RPE_FRONT_FACE_COUNTER_CLOCKWISE
};

enum PolygonMode
{
    RPE_POLYGON_MODE_FILL,
    RPE_POLYGON_MODE_LINE,
    RPE_POLYGON_MODE_POINT
};

enum CompareOp
{
    RPE_COMPARE_OP_NEVER,
    RPE_COMPARE_OP_LESS,
    RPE_COMPARE_OP_EQUAL,
    RPE_COMPARE_OP_LESS_OR_EQUAL,
    RPE_COMPARE_OP_GREATER,
    RPE_COMPARE_OP_NOT_EQUAL,
    RPE_COMPARE_OP_GREATER_OR_EQUAL,
    RPE_COMPARE_OP_ALWAYS
};

enum PrimitiveTopology
{
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
    TriangleFan,
    LineListWithAdjacency,
    LineStripWithAdjacency,
    TriangleListWithAdjacency,
    TriangleStripWithAdjacency,
    PatchList
};

enum BufferElementType
{
    RPE_BACKEND_BUFFER_ELEMENT_TYPE_UINT,
    RPE_BACKEND_BUFFER_ELEMENT_TYPE_INT,
    RPE_BACKEND_BUFFER_ELEMENT_TYPE_INT2,
    RPE_BACKEND_BUFFER_ELEMENT_TYPE_INT3,
    RPE_BACKEND_BUFFER_ELEMENT_TYPE_INT4,
    RPE_BACKEND_BUFFER_ELEMENT_TYPE_FLOAT,
    RPE_BACKEND_BUFFER_ELEMENT_TYPE_FLOAT2,
    RPE_BACKEND_BUFFER_ELEMENT_TYPE_FLOAT3,
    RPE_BACKEND_BUFFER_ELEMENT_TYPE_FLOAT4,
    RPE_BACKEND_BUFFER_ELEMENT_TYPE_MAT3,
    RPE_BACKEND_BUFFER_ELEMENT_TYPE_MAT4,
    RPE_BACKEND_BUFFER_ELEMENT_TYPE_STRUCT
};

enum ShaderStage
{
    RPE_BACKEND_SHADER_STAGE_VERTEX,
    RPE_BACKEND_SHADER_STAGE_FRAGMENT,
    RPE_BACKEND_SHADER_STAGE_TESSE_CON,
    RPE_BACKEND_SHADER_STAGE_TESSE_EVAL,
    RPE_BACKEND_SHADER_STAGE_GEOM,
    RPE_BACKEND_SHADER_STAGE_COMPUTE
};
#define RPE_BACKEND_SHADER_STAGE_MAX_COUNT 6

enum TextureFormat
{
    RPE_BACKEND_TEXTURE_FORMAT_R8,
    RPE_BACKEND_TEXTURE_FORMAT_R16F,
    RPE_BACKEND_TEXTURE_FORMAT_R32F,
    RPE_BACKEND_TEXTURE_FORMAT_R32U,
    RPE_BACKEND_TEXTURE_FORMAT_RG8,
    RPE_BACKEND_TEXTURE_FORMAT_RG16F,
    RPE_BACKEND_TEXTURE_FORMAT_RG32F,
    RPE_BACKEND_TEXTURE_FORMAT_RGB8,
    RPE_BACKEND_TEXTURE_FORMAT_RGB16F,
    RPE_BACKEND_TEXTURE_FORMAT_RGB32F,
    RPE_BACKEND_TEXTURE_FORMAT_RGBA8,
    RPE_BACKEND_TEXTURE_FORMAT_RGBA16F,
    RPE_BACKEND_TEXTURE_FORMAT_RGBA32F,
    RPE_BACKEND_TEXTURE_FORMAT_UNDEFINED
};

enum IndexBufferType
{
    Uint32,
    Uint16
};

enum ImageUsage
{
    Sampled = 1 << 0,
    Storage = 1 << 1,
    ColourAttach = 1 << 2,
    DepthAttach = 1 << 3,
    InputAttach = 1 << 4,
    Src = 1 << 5,
    Dst = 1 << 6
};

enum LoadClearFlags
{
    RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_LOAD,
    RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_CLEAR,
    RPE_BACKEND_RENDERPASS_LOAD_CLEAR_FLAG_DONTCARE
};

enum StoreClearFlags
{
    RPE_BACKEND_RENDERPASS_STORE_CLEAR_FLAG_STORE,
    RPE_BACKEND_RENDERPASS_STORE_CLEAR_FLAG_DONTCARE
};

typedef struct TextureSamplerParams
{
    enum SamplerFilter min;
    enum SamplerFilter mag;
    enum SamplerAddressMode addr_u;
    enum SamplerAddressMode addr_v;
    enum SamplerAddressMode addr_w;
    enum CompareOp compare_op;
    float anisotropy;
    uint32_t mip_levels;
    VkBool32 enable_compare;
    VkBool32 enable_anisotropy;
} sampler_params_t;

#endif
