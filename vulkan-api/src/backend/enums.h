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

#ifndef __VKAPI_BACKEND_ENUMS_H__
#define __VKAPI_BACKEND_ENUMS_H__

#include <stdint.h>

enum lendFactor
{
    Zero,
    One,
    SrcColour,
    OneMinusSrcColour,
    DstColour,
    OneMinusDstColour,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    ConstantColour,
    OneMinusConstantColour,
    ConstantAlpha,
    OneMinusConstantAlpha,
    SrcAlphaSaturate
};

enum BlendOp
{
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max,
};

enum BlendFactorPresets
{
    Translucent
};

enum SamplerAddressMode
{
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder,
    MirrorClampToEdge
};

enum SamplerFilter
{
    Nearest,
    Linear,
    Cubic
};

enum CullMode
{
    Back,
    Front,
    None
};

enum CompareOp
{
    Never,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always
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
    Uint,
    Int,
    Int2,
    Int3,
    Int4,
    Float,
    Float2,
    Float3,
    Float4,
    Mat3,
    Mat4,
    Struct
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
    R8,
    R16F,
    R32F,
    R32U,
    RG8,
    RG16F,
    RG32F,
    RGB8,
    RGB16F,
    RGB32F,
    RGBA8,
    RGBA16F,
    RGBA32F,
    Undefined
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

#endif
