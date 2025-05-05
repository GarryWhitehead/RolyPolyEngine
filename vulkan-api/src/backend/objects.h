/* Copyright (c) 2024-2025 Garry Whitehead
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

#ifndef __BACKEND_OBJECTS_H__
#define __BACKEND_OBJECTS_H__

#include "enums.h"

#include <vulkan-api/common.h>

typedef struct TextureSamplerParams
{
    enum SamplerFilter min;
    enum SamplerFilter mag;
    enum SamplerAddressMode addr_u;
    enum SamplerAddressMode addr_v;
    enum SamplerAddressMode addr_w;
    enum CompareOp compare_op;
    float anisotropy;
    // NOTE: When creating sampled textures, this value is automatically set by the createTex2d
    // function based upon the level count specified in the texture info.
    uint32_t mip_levels;
    VkBool32 enable_compare;
    VkBool32 enable_anisotropy;
} sampler_params_t;

typedef struct Rect2D
{
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
} rpe_rect2d_t;

typedef struct ViewPort
{
    rpe_rect2d_t rect;
    float min_depth;
    float max_depth;
} rpe_viewport_t;

#endif