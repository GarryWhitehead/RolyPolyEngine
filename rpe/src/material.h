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

#ifndef __RPE_PRIV_MATERIAL_H__
#define __RPE_PRIV_MATERIAL_H__

#include "backend/enums.h"
#include "rpe/material.h"

#include <utility/maths.h>
#include <vulkan-api/descriptor_cache.h>
#include <vulkan-api/driver.h>
#include <vulkan-api/pipeline_cache.h>
#include <vulkan-api/program_manager.h>
#include <vulkan-api/shader.h>
#include <vulkan-api/texture.h>

// Forward declarations.
typedef struct Engine rpe_engine_t;
typedef struct Mesh rpe_mesh_t;

typedef struct MaterialHandle
{
    uint32_t id;
} rpe_mat_handle_t;

struct BufferInfo
{
    buffer_handle_t handle;
    enum ShaderStage stage;
};

typedef struct Material
{
    rpe_mat_handle_t handle;

    // Note: The order of constants must match that used by the material fragment shader.
    struct MeshConstants
    {
        int has_skin;
        int has_normal;
    } mesh_consts;

    struct MaterialConstants
    {
        int has_alpha_mask;
        int has_base_colour_sampler;
        int has_alpha_mask_cutoff;
        int pipeline_type;
        int has_mr_sampler;
        int has_diffuse_sampler;
        int has_diffuse_factor;
        int has_normal_sampler;
        int has_occlusion_sampler;
        int has_emissive_sampler;
        int has_uv;
        int has_normal;
        int has_tangent;
        int has_colour_attr;
    } material_consts;

    // A representation of the data buffer found in the shader.
    struct DrawData
    {
        math_vec4f emissive_factor;
        math_vec4f base_colour_factor;
        math_vec4f diffuse_factor;
        math_vec4f specular_factor;
        float alpha_mask_cut_off;
        float alpha_mask;
        float roughness_factor;
        float metallic_factor;
        // These are the index ids for each texture into the resource cache.
        uint32_t image_indices[RPE_MATERIAL_IMAGE_TYPE_COUNT];
        uint32_t uv_indices[RPE_MATERIAL_IMAGE_TYPE_COUNT];
    } material_draw_data;

    struct MaterialKey
    {
        enum PolygonMode polygon_mode;
        enum FrontFace front_face;
        enum CullMode cull_mode;
        bool depth_test_enable;
        bool depth_write_enable;
        enum CompareOp depth_compare_op;
        enum PrimitiveTopology topo;

        struct MaterialBlendFactor blend_state;
        struct MaterialConstants constants;
    } material_key;

    bool double_sided;
    uint8_t view_layer;

    // ============== vulkan backend stuff =======================

    /// details for rendering this material
    shader_prog_bundle_t* program_bundle;

    /// the sampler descriptor bindings
    desc_bind_info_t samplers[VKAPI_PIPELINE_MAX_SAMPLER_BIND_COUNT];

    /// Backend buffer handles.
    arena_dyn_array_t buffers;
} rpe_material_t;

rpe_material_t rpe_material_init(rpe_engine_t* e, arena_t* arena);

void rpe_material_add_buffer(rpe_material_t* m, buffer_handle_t handle, enum ShaderStage stage);

void rpe_material_set_scissor(
    rpe_material_t* m, uint32_t width, uint32_t height, uint32_t xOffset, uint32_t yOffset);
void rpe_material_set_viewport(
    rpe_material_t* m, uint32_t width, uint32_t height, float minDepth, float maxDepth);

void rpe_material_update_vertex_constants(rpe_material_t* mat, rpe_mesh_t* mesh);

uint32_t rpe_material_max_mipmaps(uint32_t width, uint32_t height);

#endif