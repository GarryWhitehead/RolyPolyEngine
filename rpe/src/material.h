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

#ifndef __RPE_MATERIAL_H__
#define __RPE_MATERIAL_H__

#include "backend/enums.h"

#include <utility/maths.h>
#include <vulkan-api/descriptor_cache.h>
#include <vulkan-api/driver.h>
#include <vulkan-api/pipeline_cache.h>
#include <vulkan-api/program_manager.h>
#include <vulkan-api/shader.h>
#include <vulkan-api/texture.h>

#define VertexUboBindPoint 4
#define FragmentUboBindPoint 5
#define MaxSamplerCount 6

// Forward declarations.
typedef struct RenderPrimitive rpe_render_primitive_t;
typedef struct Engine rpe_engine_t;

enum MaterialImageType
{
    RPE_MATERIAL_IMAGE_TYPE_BASE_COLOR,
    RPE_MATERIAL_IMAGE_TYPE_NORMAL,
    RPE_MATERIAL_IMAGE_TYPE_METALLIC_ROUGHNESS,
    RPE_MATERIAL_IMAGE_TYPE_EMISSIVE,
    RPE_MATERIAL_IMAGE_TYPE_OCCLUSION
};
#define RPE_MATERIAL_IMAGE_TYPE_COUNT 5

enum MaterialPipeline
{
    RPE_MATERIAL_PIPELINE_MR,
    RPE_MATERIAL_PIPELINE_SPECULAR,
    RPE_MATERIAL_PIPELINE_NONE
};

typedef struct MaterialHandle
{
    uint32_t id;
} rpe_mat_handle_t;

enum SamplerType
{
    RPE_SAMPLER_TYPE_2D,
    RPE_SAMPLER_TYPE_3D,
    RPE_SAMPLER_TYPE_CUBE
};

typedef struct MappedTexture
{
    void* buffer;
    VkFormat format;
    uint32_t width;
    uint32_t height;
    uint32_t mip_levels;
    uint32_t face_count;
    texture_handle_t backend_handle;
} rpe_mapped_texture_t;

struct BufferInfo
{
    buffer_handle_t handle;
    enum ShaderStage stage;
};

struct MaterialBlendFactor
{
    bool state;
    enum BlendFactor src_color;
    enum BlendFactor dstColor;
    enum BlendOp colour;
    enum BlendFactor srcAlpha;
    enum BlendFactor dstAlpha;
    enum BlendOp alpha;
};

// This is used and set by the scene update().
struct MaterialPushData
{
    uint32_t draw_idx;
};

typedef struct Material
{
    // Handle created by the RenderableManager to ourself. Used for updating the shader push
    // constant.
    rpe_mat_handle_t handle;

    // Note: The order of constants must match that used by the material fragment shader.
    struct MeshConstants
    {
        int has_skin;
    } mesh_consts;
    struct MaterialConstants
    {
        int has_alpha_mask;
        int has_alpha_mask_cutoff;
        int has_base_colour_sampler;
        int has_base_colour_factor;
        int has_diffuse_sampler;
        int has_diffuse_factor;
        int has_emissive_sampler;
        int has_emissive_factor;
        int has_normal_sampler;
        int has_mr_sampler;
        int has_occlusion_sampler;
        int pipeline_type;
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

        uint32_t colour;
        uint32_t normal;
        uint32_t mr;
        uint32_t diffuse;
        uint32_t emissive;
        uint32_t occlusion;
        uint32_t pad0[2];
    } material_draw_data;

    bool double_sided;
    uint8_t view_layer;
    // Cache the sort key here to save wasting calculating this each frame.
    uint64_t sort_key;

    // ============== vulkan backend stuff =======================

    /// details for rendering this material
    shader_prog_bundle_t* program_bundle;

    /// the sampler descriptor bindings
    desc_bind_info_t samplers[VKAPI_PIPELINE_MAX_SAMPLER_BIND_COUNT];

    /// Backend buffer handles.
    arena_dyn_array_t buffers;
} rpe_material_t;

rpe_material_t* rpe_material_init(rpe_engine_t* e, arena_t* arena);

void rpe_material_set_pipeline(rpe_material_t* m, enum MaterialPipeline pipeline);

void rpe_material_add_image_texture(
    rpe_material_t* m,
    vkapi_driver_t* driver,
    rpe_mapped_texture_t* tex,
    enum MaterialImageType type,
    enum ShaderStage stage,
    sampler_params_t params,
    uint32_t binding);

void rpe_material_add_buffer(rpe_material_t* m, buffer_handle_t handle, enum ShaderStage stage);

void rpe_material_set_render_prim(rpe_material_t* m, rpe_render_primitive_t* p);

void rpe_material_set_blend_factors(rpe_material_t* m, struct MaterialBlendFactor factors);

void rpe_material_set_double_sided_state(rpe_material_t* m, bool state);
void rpe_material_set_test_enable(rpe_material_t* m, bool state);
void rpe_material_set_write_enable(rpe_material_t* m, bool state);
void rpe_material_set_depth_compare_op(rpe_material_t* m, VkCompareOp op);
void rpe_material_set_polygon_mode(rpe_material_t* m, VkPolygonMode mode);
void rpe_material_set_front_face(rpe_material_t* m, VkFrontFace face);
void rpe_material_set_cull_mode(rpe_material_t* m, VkCullModeFlagBits mode);

void rpe_material_set_scissor(
    rpe_material_t* m, uint32_t width, uint32_t height, uint32_t xOffset, uint32_t yOffset);
void rpe_material_set_viewport(
    rpe_material_t* m, uint32_t width, uint32_t height, float minDepth, float maxDepth);

void rpe_material_set_view_layer(rpe_material_t* m, uint8_t layer);

void rpe_material_set_base_colour_factor(rpe_material_t* m, math_vec4f* f);
void rpe_material_set_diffuse_factor(rpe_material_t* m, math_vec4f* f);
void rpe_material_set_bas_colour_factor(rpe_material_t* m, math_vec4f* f);
void rpe_material_set_emissive_factor(rpe_material_t* m, math_vec4f* f);
void rpe_material_set_roughness_factor(rpe_material_t* m, float f);
void rpe_material_set_metallic_factor(rpe_material_t* m, float f);
void rpe_material_set_alpha_mask(rpe_material_t* m, float mask);
void rpe_material_set_alpha_cutoff(rpe_material_t* m, float co);


#endif