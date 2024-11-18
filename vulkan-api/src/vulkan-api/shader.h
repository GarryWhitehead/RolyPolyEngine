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

#ifndef __VKAPI_SHADER_H__
#define __VKAPI_SHADER_H__

#include "backend/enums.h"
#include "common.h"
#include "pipeline.h"

#include <utility/arena.h>
#include <utility/string.h>

#define VKAPI_SHADER_MAX_STAGE_INPUTS 15
#define VKAPI_SHADER_MAX_STAGE_OUTPUTS 15
#define VKAPI_SHADER_MAX_DESC_LAYOUTS 50

extern const size_t sizeof_shader_t;

// Forward declarations.
typedef struct VkApiContext vkapi_context_t;
typedef struct Shader shader_t;

typedef struct Attribute
{
    uint32_t location;
    uint32_t stride;
    VkFormat format;
} shader_attr_t;

typedef struct DescriptorLayout
{
    string_t name;
    uint32_t binding;
    uint32_t set;
    size_t range;
    VkDescriptorType type;
    VkShaderStageFlags stage;
} shader_desc_layout_t;

/**
 Filled through reflection of the shader code, this struct holds
 information required to create the relevant Vulkan objects.
 */
typedef struct ShaderBinding
{
    shader_attr_t stage_inputs[VKAPI_SHADER_MAX_STAGE_INPUTS];
    shader_attr_t stage_outputs[VKAPI_SHADER_MAX_STAGE_OUTPUTS];
    shader_desc_layout_t desc_layouts[VKAPI_SHADER_MAX_DESC_LAYOUTS];

    struct SpecializationConst
    {
        uint32_t id;
        uint32_t size;
        uint32_t offset;
    } spec_consts[VKAPI_PIPELINE_MAX_SPECIALIZATION_COUNT];

    uint32_t stage_input_count;
    uint32_t stage_output_count;
    uint32_t desc_layout_count;
    uint32_t spec_const_count;
    size_t push_block_size;
} shader_binding_t;

typedef struct SpirVBinary
{
    /// SPIR-V 32-bit words.
    uint32_t* words;
    // number of words in SPIR-V binary.
    size_t size;
} spirv_binary_t;

typedef struct Shader
{
    /// All the bindings for this shader - generated via the @p reflect call.
    shader_binding_t resource_binding;
    /// A vulkan shader module object for use with a pipeline.
    VkShaderModule module;
    /// The stage of this shader see @p backend::ShaderStage.
    enum ShaderStage stage;
    /// Create info used by the graphics/compute pipeline.
    VkPipelineShaderStageCreateInfo create_info;
} shader_t;

/**
 Initialise a shader for a given stage.
 @param stage The stage the shader will represent.
 @param arena A arena allocator.
 @return A pointer to the initialised shader.
 */
shader_t* shader_init(enum ShaderStage stage, arena_t* arena);

spirv_binary_t shader_load_spirv(const char* filename, arena_t* arena);

/**
 Compile a shader code block to SPIRV and create the Vk shader module.
 @param shader The shader state.
 @param shader_code The shdaer code text to compile.
 @param filename The filename form where the @sa shader_code was derived.
 @param arena An arena allocator.
 @return
 */
spirv_binary_t shader_compile(
    shader_t* shader,
    const char* shader_code,
    const char* filename,
    arena_t* arena);

void shader_create_vk_module(shader_t* shader, vkapi_context_t* context, spirv_binary_t bin);

string_t shader_stage_to_string(enum ShaderStage stage, arena_t* arena);

/**
 Convert stage flags from RPE to Vk format.
 @param stage The stage to convert.
 @return The stage in Vk format.
 */
VkShaderStageFlagBits shader_vk_stage_flag(enum ShaderStage stage);

/**
 Conduct reflection on the SPIRV byte code.
 @param shader The shader state.
 @param spirv The SPIRV shader to perform reflection on in 32-bit word format.
 @param word_count The size of the shader code.
 @param arena A arena allocator.
 */
void shader_reflect_spirv(shader_t* shader, uint32_t* spirv, uint32_t word_count, arena_t* arena);



#endif