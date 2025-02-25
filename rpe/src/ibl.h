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

#ifndef __PRIV_IBL_H__
#define __PRIV_IBL_H__

#include "material.h"
#include "rpe/ibl.h"

#include <stdint.h>
#include <vulkan-api/program_manager.h>

typedef struct ShaderProgramBundle shader_prog_bundle_t;
typedef struct Scene rpe_scene_t;
typedef struct Renderer rpe_renderer_t;
typedef struct Camera rpe_camera_t;
typedef struct Renderable rpe_renderable_t;
typedef struct Mesh rpe_mesh_t;

#define RPE_IBL_EQIRECT_CUBEMAP_DIMENSIONS 512
#define RPE_IBL_IRRADIANCE_ENV_MAP_DIMENSIONS 64
#define RPE_IBL_SPECULAR_ENV_MAP_DIMENSIONS 512

struct BrdfUBO
{
    uint32_t sample_count;
};

struct FaceviewUBO
{
    math_mat4f face_views[6];
};

struct SpecularFragUBO
{
    uint32_t sample_count;
};

struct IblBundle
{
    shader_handle_t handles[2];
    shader_prog_bundle_t* bundle;
};

typedef struct Ibl
{
    struct IblBundle brdf;
    struct IblBundle specular;
    struct IblBundle irradiance_envmap;

    buffer_handle_t brdf_ubo;
    buffer_handle_t faceview_ubo;
    buffer_handle_t specular_frag_ubo;
    buffer_handle_t cubemap_vertices;
    buffer_handle_t cubemap_indices;

    texture_handle_t tex_cube_map;
    texture_handle_t tex_irradiance_map;
    texture_handle_t tex_specular_map;
    texture_handle_t tex_brdf_lut;

    rpe_renderer_t* renderer;
    rpe_camera_t* camera;

    struct PreFilterOptions options;
} ibl_t;

#endif