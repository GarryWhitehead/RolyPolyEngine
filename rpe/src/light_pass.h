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

#ifndef __RPE_LIGHT_PASS_H__
#define __RPE_LIGHT_PASS_H__

#include <render_graph/render_graph_handle.h>
#include <vulkan-api/common.h>

#define RPE_LIGHT_PASS_SAMPLER_POS_BINDING 0
#define RPE_LIGHT_PASS_SAMPLER_COLOUR_BINDING 1
#define RPE_LIGHT_PASS_SAMPLER_NORMAL_BINDING 2
#define RPE_LIGHT_PASS_SAMPLER_PBR_BINDING 3
#define RPE_LIGHT_PASS_SAMPLER_EMISSIVE_BINDING 4
#define RPE_LIGHT_PASS_SAMPLER_BDRF_BINDING 5
#define RPE_LIGHT_PASS_SAMPLER_IRRADIANCE_ENVMAP_BINDING 6
#define RPE_LIGHT_PASS_SAMPLER_SPECULAR_ENVMAP_BINDING 7

typedef struct ShaderProgramBundle shader_prog_bundle_t;
typedef struct LightManager rpe_light_manager_t;
typedef struct RenderGraph render_graph_t;

struct LightPassData
{
    rg_handle_t rt;
    rg_handle_t light;
    rg_handle_t depth;
    // Gbuffer inputs.
    rg_handle_t position;
    rg_handle_t normal;
    rg_handle_t colour;
    rg_handle_t pbr;
    rg_handle_t emissive;
    // Passed from setup local data.
    shader_prog_bundle_t* prog_bundle;
};

struct LightLocalData
{
    uint32_t width;
    uint32_t height;
    VkFormat depth_format;
    shader_prog_bundle_t* prog_bundle;
};

rg_handle_t rpe_light_pass_render(
    rpe_light_manager_t* lm,
    render_graph_t* rg,
    uint32_t width,
    uint32_t height,
    VkFormat depth_format);

#endif