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

#ifndef __VKAPI_PROGRAM_MANAGER_H__
#define __VKAPI_PROGRAM_MANAGER_H__

#include "common.h"
#include "descriptor_cache.h"
#include "driver.h"
#include "pipeline.h"

#include <utility/arena.h>
#include <utility/hash_set.h>
#include <utility/string.h>

// Forward declarations.
typedef struct Shader shader_t;
typedef struct PipelineLayout vkapi_pl_layout_t;
typedef struct TextureHandle texture_handle_t;

#define VKAPI_INVALID_SHADER_HANDLE UINT32_MAX

typedef struct ShaderHandle
{
    uint32_t id;
} shader_handle_t;

bool vkapi_is_valid_shader_handle(shader_handle_t h);
void vkapi_invalidate_shader_handle(shader_handle_t* h);

typedef struct DescriptorBindInfo
{
    uint32_t binding;
    buffer_handle_t buffer;
    uint32_t size;
    VkDescriptorType type;
} desc_bind_info_t;

typedef struct ShaderProgramBundle
{
    struct RasterState
    {
        VkCullModeFlagBits cull_mode;
        VkPolygonMode polygon_mode;
        VkFrontFace front_face;
        VkBool32 depth_clamp_enable;
    } raster_state;

    struct StencilState
    {
        VkStencilOp fail_op;
        VkStencilOp pass_op;
        VkStencilOp depth_fail_op;
        VkStencilOp stencil_fail_op;
        VkCompareOp compare_op;
        uint32_t compare_mask;
        uint32_t write_mask;
        uint32_t reference;
    } stencil_state;

    struct DepthStencilState
    {
        struct StencilState stencil_state;

        // Depth state.
        VkBool32 test_enable;
        VkBool32 write_enable;
        VkBool32 stencil_test_enable;
        VkCompareOp compare_op;

        // Only processed if the above is true.
        struct StencilState front;
        struct StencilState back;
    } ds_state;

    struct BlendFactorState
    {
        VkBool32 blend_enable;
        VkBlendFactor src_colour;
        VkBlendFactor dst_colour;
        VkBlendOp colour;
        VkBlendFactor src_alpha;
        VkBlendFactor dst_alpha;
        VkBlendOp alpha;
    } blend_state;

    struct ProgramPrimitive
    {
        VkPrimitiveTopology topology;
        VkBool32 prim_restart;
    } render_prim;

    struct PushBlockBindParams
    {
        uint32_t binding;
        uint32_t range;
        VkShaderStageFlags stage;
        void* data;
    } push_blocks[RPE_BACKEND_SHADER_STAGE_MAX_COUNT];

    struct SpecConstParams
    {
        VkSpecializationMapEntry entries[VKAPI_PIPELINE_MAX_SPECIALIZATION_COUNT];
        uint32_t entry_count;
        void* data;
        uint32_t data_size;
    } spec_const_params[RPE_BACKEND_SHADER_STAGE_MAX_COUNT];

    // Index into the resource cache for the texture for each attachment.
    struct ImageSamplerParams
    {
        texture_handle_t handle;
        VkSampler sampler;
    } image_samplers[VKAPI_PIPELINE_MAX_SAMPLER_BIND_COUNT];
    bool use_bound_samplers;
    texture_handle_t storage_images[VKAPI_PIPELINE_MAX_STORAGE_IMAGE_BOUND_COUNT];

    VkDescriptorSetLayout desc_layouts[VKAPI_PIPELINE_MAX_DESC_SET_COUNT];

    VkVertexInputAttributeDescription vert_attrs[VKAPI_PIPELINE_MAX_VERTEX_ATTR_COUNT];
    VkVertexInputBindingDescription vert_bind_desc[VKAPI_PIPELINE_MAX_INPUT_BIND_COUNT];

    // We keep a record of descriptors here and their binding info for
    // use at the pipeline binding draw stage.
    desc_bind_info_t ubos[VKAPI_PIPELINE_MAX_UBO_BIND_COUNT];
    desc_bind_info_t ssbos[VKAPI_PIPELINE_MAX_SSBO_BIND_COUNT];

    VkRect2D scissor;
    VkViewport viewport;
    size_t tesse_vert_count;

    shader_handle_t shaders[RPE_BACKEND_SHADER_STAGE_MAX_COUNT];

    // Descriptor set binding mappings for creating a pipeline layout.
    VkDescriptorSetLayoutBinding desc_bindings[VKAPI_PIPELINE_MAX_DESC_SET_COUNT]
                                              [VKAPI_PIPELINE_MAX_DESC_SET_LAYOUT_BINDING_COUNT];
    int desc_binding_counts[VKAPI_PIPELINE_MAX_DESC_SET_COUNT];

} shader_prog_bundle_t;

typedef struct ProgramCache
{
    /// Fully compiled, complete shader programs.
    arena_dyn_array_t program_bundles;

    // This is where individual shaders are cached until required to assemble
    // into a complete shader program.
    arena_dyn_array_t shaders;

} program_cache_t;

/* Shader bundle functions */

void shader_bundle_update_ubo_desc(
    shader_prog_bundle_t* bundle, uint32_t binding, buffer_handle_t buffer);

void shader_bundle_update_ssbo_desc(
    shader_prog_bundle_t* bundle, uint32_t binding, buffer_handle_t buffer, uint32_t size);

void shader_bundle_add_desc_binding(
    shader_prog_bundle_t* bundle, uint32_t size, uint32_t binding, VkDescriptorType type);

void shader_bundle_add_image_sampler(
    shader_prog_bundle_t* bundle, vkapi_driver_t* driver, texture_handle_t handle, uint8_t binding);

void shader_bundle_add_storage_image(
    shader_prog_bundle_t* bundle, texture_handle_t handle, uint8_t binding);

void shader_bundle_set_viewport(
    shader_prog_bundle_t* bundle, uint32_t width, uint32_t height, float minDepth, float maxDepth);

void shader_bundle_set_scissor(
    shader_prog_bundle_t* bundle,
    uint32_t width,
    uint32_t height,
    uint32_t x_offset,
    uint32_t y_offset);

void shader_bundle_add_render_primitive(
    shader_prog_bundle_t* bundle, VkPrimitiveTopology topo, VkBool32 prim_restart);
void shader_bundle_set_cull_mode(shader_prog_bundle_t* bundle, enum CullMode mode);
void shader_bundle_set_depth_read_write_state(
    shader_prog_bundle_t* bundle, bool test_state, bool write_state, enum CompareOp depth_op);
void shader_bundle_set_depth_clamp_state(shader_prog_bundle_t* bundle, bool state);

void shader_bundle_update_spec_const_data(
    shader_prog_bundle_t* bundle, uint32_t data_size, void* data, enum ShaderStage stage);

void shader_bundle_get_shader_stage_create_info_all(
    shader_prog_bundle_t* b, vkapi_driver_t* driver, VkPipelineShaderStageCreateInfo* out);

VkPipelineShaderStageCreateInfo shader_bundle_get_shader_stage_create_info(
    shader_prog_bundle_t* b, vkapi_driver_t* driver, enum ShaderStage stage);

void shader_bundle_update_descs_from_reflection(
    shader_prog_bundle_t* bundle, vkapi_driver_t* driver, shader_handle_t handle, arena_t* arena);

void shader_bundle_add_vertex_input_binding(
    shader_prog_bundle_t* bundle,
    shader_handle_t handle,
    vkapi_driver_t* driver,
    uint32_t firstIndex,
    uint32_t lastIndex,
    uint32_t binding,
    VkVertexInputRate input_rate);

void shader_bundle_create_push_block(
    shader_prog_bundle_t* bundle, size_t size, enum ShaderStage stage);

/* Program cache functions */

program_cache_t* program_cache_init(arena_t* arena);

void program_cache_destroy(program_cache_t* c, vkapi_driver_t* driver);

shader_handle_t program_cache_compile_shader(
    program_cache_t* c,
    vkapi_context_t* context,
    const char* shader_code,
    enum ShaderStage stage,
    arena_t* arena);

shader_handle_t program_cache_from_spirv(
    program_cache_t* c,
    vkapi_context_t* context,
    const char* filename,
    enum ShaderStage stage,
    arena_t* arena);

shader_prog_bundle_t* program_cache_create_program_bundle(program_cache_t* c, arena_t* arena);

shader_t* program_cache_get_shader(program_cache_t* c, shader_handle_t handle);

void program_cache_destroy(program_cache_t* c, vkapi_driver_t* driver);

#endif
