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

#ifndef __RPE_SHADOW_PASS_H__
#define __RPE_SHADOW_PASS_H__

#include <render_graph/render_graph_handle.h>
#include <stdint.h>
#include <vulkan-api/common.h>

typedef struct ShaderProgramBundle shader_prog_bundle_t;
typedef struct ShadowManager rpe_shadow_manager_t;
typedef struct RenderGraph render_graph_t;
typedef struct Scene rpe_scene_t;

/** Cascade shadow pass data **/

struct ShadowLocalData
{
    uint32_t width;
    uint32_t height;
    uint32_t cascade_count;
    VkFormat depth_format;
    shader_prog_bundle_t* prog_bundle;
    rpe_scene_t* scene;
};

struct ShadowPassData
{
    rg_handle_t rt;
    rg_handle_t depth;
    // Passed from setup local data.
    shader_prog_bundle_t* prog_bundle;
    rpe_scene_t* scene;
};

rg_handle_t rpe_shadow_pass_render(
    rpe_shadow_manager_t* sm,
    render_graph_t* rg,
    rpe_scene_t* scene,
    uint32_t dimensions,
    VkFormat depth_format);

/** Shadow cascade debug data **/

struct CascadeDebugLocalData
{
    uint32_t width;
    uint32_t height;
    shader_prog_bundle_t* prog_bundle;
    rpe_scene_t* scene;
};

struct CascadeDebugPassData
{
    rg_handle_t rt;
    rg_handle_t colour;
    // Cascade shadow map for visualising.
    rg_handle_t cascade_map;
    // Colour map from the light pass.
    rg_handle_t light_colour;
    // Passed from setup local data.
    shader_prog_bundle_t* prog_bundle;
    rpe_scene_t* scene;
};

rg_handle_t rpe_cascade_shadow_debug_render(
    rpe_shadow_manager_t* sm, render_graph_t* rg, uint32_t width, uint32_t height);

#endif