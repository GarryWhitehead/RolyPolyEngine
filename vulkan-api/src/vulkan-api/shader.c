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

#include "shader.h"

#include "descriptor_cache.h"
#include "driver.h"
#include "pipeline_cache.h"

#include <glslang/Include/glslang_c_interface.h>
#include <log.h>
#include <spirv_cross_c.h>
#include <string.h>
#include <utility/arena.h>


const size_t sizeof_shader_t = sizeof(struct Shader);

glslang_resource_t* glslang_create_resource(arena_t* arena)
{
    glslang_resource_t* resources = ARENA_MAKE_STRUCT(arena, glslang_resource_t, ARENA_ZERO_MEMORY);

    resources->max_lights = 32;
    resources->max_clip_planes = 6;
    resources->max_texture_units = 32;
    resources->max_texture_coords = 32;
    resources->max_vertex_attribs = 64;
    resources->max_vertex_uniform_components = 4096;
    resources->max_varying_floats = 64;
    resources->max_vertex_texture_image_units = 32;
    resources->max_combined_texture_image_units = 80;
    resources->max_texture_image_units = 32;
    resources->max_fragment_uniform_components = 4096;
    resources->max_draw_buffers = 32;
    resources->max_vertex_uniform_vectors = 128;
    resources->max_varying_vectors = 8;
    resources->max_fragment_uniform_vectors = 16;
    resources->max_vertex_output_vectors = 16;
    resources->max_fragment_input_vectors = 15;
    resources->min_program_texel_offset = -8;
    resources->max_program_texel_offset = 7;
    resources->max_clip_distances = 8;
    resources->max_compute_work_group_count_x = 65535;
    resources->max_compute_work_group_count_y = 65535;
    resources->max_compute_work_group_count_z = 65535;
    resources->max_compute_work_group_size_x = 1024;
    resources->max_compute_work_group_size_y = 1024;
    resources->max_compute_work_group_size_z = 64;
    resources->max_compute_uniform_components = 1024;
    resources->max_compute_texture_image_units = 16;
    resources->max_compute_image_uniforms = 8;
    resources->max_compute_atomic_counters = 8;
    resources->max_compute_atomic_counter_buffers = 1;
    resources->max_varying_components = 60;
    resources->max_vertex_output_components = 64;
    resources->max_geometry_input_components = 64;
    resources->max_geometry_output_components = 128;
    resources->max_fragment_input_components = 128;
    resources->max_image_units = 8;
    resources->max_combined_image_units_and_fragment_outputs = 8;
    resources->max_combined_shader_output_resources = 8;
    resources->max_image_samples = 0;
    resources->max_vertex_image_uniforms = 0;
    resources->max_tess_control_image_uniforms = 0;
    resources->max_tess_evaluation_image_uniforms = 0;
    resources->max_geometry_image_uniforms = 0;
    resources->max_fragment_image_uniforms = 8;
    resources->max_combined_image_uniforms = 8;
    resources->max_geometry_texture_image_units = 16;
    resources->max_geometry_output_vertices = 256;
    resources->max_geometry_total_output_components = 1024;
    resources->max_geometry_uniform_components = 1024;
    resources->max_geometry_varying_components = 64;
    resources->max_tess_control_input_components = 128;
    resources->max_tess_control_output_components = 128;
    resources->max_tess_control_texture_image_units = 16;
    resources->max_tess_control_uniform_components = 1024;
    resources->max_tess_control_total_output_components = 4096;
    resources->max_tess_evaluation_input_components = 128;
    resources->max_tess_evaluation_output_components = 128;
    resources->max_tess_evaluation_texture_image_units = 16;
    resources->max_tess_evaluation_uniform_components = 1024;
    resources->max_tess_patch_components = 120;
    resources->max_patch_vertices = 32;
    resources->max_tess_gen_level = 64;
    resources->max_viewports = 16;
    resources->max_vertex_atomic_counters = 0;
    resources->max_tess_control_atomic_counters = 0;
    resources->max_tess_evaluation_atomic_counters = 0;
    resources->max_geometry_atomic_counters = 0;
    resources->max_fragment_atomic_counters = 8;
    resources->max_combined_atomic_counters = 8;
    resources->max_atomic_counter_bindings = 1;
    resources->max_vertex_atomic_counter_buffers = 0;
    resources->max_tess_control_atomic_counter_buffers = 0;
    resources->max_tess_evaluation_atomic_counter_buffers = 0;
    resources->max_geometry_atomic_counter_buffers = 0;
    resources->max_fragment_atomic_counter_buffers = 1;
    resources->max_combined_atomic_counter_buffers = 1;
    resources->max_atomic_counter_buffer_size = 16384;
    resources->max_transform_feedback_buffers = 4;
    resources->max_transform_feedback_interleaved_components = 64;
    resources->max_cull_distances = 8;
    resources->max_combined_clip_and_cull_distances = 8;
    resources->max_samples = 4;
    resources->max_mesh_output_vertices_nv = 256;
    resources->max_mesh_output_primitives_nv = 512;
    resources->max_mesh_work_group_size_x_nv = 32;
    resources->max_mesh_work_group_size_y_nv = 1;
    resources->max_mesh_work_group_size_z_nv = 1;
    resources->max_task_work_group_size_x_nv = 32;
    resources->max_task_work_group_size_y_nv = 1;
    resources->max_task_work_group_size_z_nv = 1;
    resources->max_mesh_view_count_nv = 4;

    resources->limits.non_inductive_for_loops = 1;
    resources->limits.while_loops = 1;
    resources->limits.do_while_loops = 1;
    resources->limits.general_uniform_indexing = 1;
    resources->limits.general_attribute_matrix_vector_indexing = 1;
    resources->limits.general_varying_indexing = 1;
    resources->limits.general_sampler_indexing = 1;
    resources->limits.general_variable_indexing = 1;
    resources->limits.general_constant_matrix_vector_indexing = 1;
    return resources;
}

glslang_stage_t shader_to_glslang_type(enum ShaderStage stage)
{
    glslang_stage_t result;
    switch (stage)
    {
        case RPE_BACKEND_SHADER_STAGE_VERTEX:
            result = GLSLANG_STAGE_VERTEX;
            break;
        case RPE_BACKEND_SHADER_STAGE_FRAGMENT:
            result = GLSLANG_STAGE_FRAGMENT;
            break;
        case RPE_BACKEND_SHADER_STAGE_GEOM:
            result = GLSLANG_STAGE_GEOMETRY;
            break;
        case RPE_BACKEND_SHADER_STAGE_COMPUTE:
            result = GLSLANG_STAGE_COMPUTE;
            break;
        case RPE_BACKEND_SHADER_STAGE_TESSE_EVAL:
            result = GLSLANG_STAGE_TESSEVALUATION;
            break;
        case RPE_BACKEND_SHADER_STAGE_TESSE_CON:
            result = GLSLANG_STAGE_TESSCONTROL;
            break;
    }

    return result;
}

spirv_binary_t shader_compiler_compile(
    glslang_stage_t stage, const char* shader_src, const char* filename, arena_t* arena)
{
    const glslang_input_t input = {
        .language = GLSLANG_SOURCE_GLSL,
        .stage = stage,
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_2,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = GLSLANG_TARGET_SPV_1_5,
        .code = shader_src,
        .default_version = 100,
        .default_profile = GLSLANG_NO_PROFILE,
        .force_default_version_and_profile = false,
        .forward_compatible = false,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
        .resource = glslang_create_resource(arena)};

    glslang_initialize_process();

    glslang_shader_t* shader = glslang_shader_create(&input);

    struct SpirVBinary bin = {
        .words = NULL,
        .size = 0,
    };

    if (!glslang_shader_preprocess(shader, &input))
    {
        log_error("GLSL preprocessing failed %s\n", filename);
        log_error("%s\n", glslang_shader_get_info_log(shader));
        log_error("%s\n", glslang_shader_get_info_debug_log(shader));
        log_error("%s\n", input.code);
        glslang_shader_delete(shader);
        return bin;
    }

    if (!glslang_shader_parse(shader, &input))
    {
        log_error("GLSL parsing failed %s\n", filename);
        log_error("%s\n", glslang_shader_get_info_log(shader));
        log_error("%s\n", glslang_shader_get_info_debug_log(shader));
        log_error("%s\n", glslang_shader_get_preprocessed_code(shader));
        glslang_shader_delete(shader);
        return bin;
    }

    glslang_program_t* program = glslang_program_create();
    glslang_program_add_shader(program, shader);

    if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT))
    {
        log_error("GLSL linking failed %s\n", filename);
        log_error("%s\n", glslang_program_get_info_log(program));
        log_error("%s\n", glslang_program_get_info_debug_log(program));
        glslang_program_delete(program);
        glslang_shader_delete(shader);
        return bin;
    }

    glslang_program_SPIRV_generate(program, stage);

    bin.size = glslang_program_SPIRV_get_size(program);
    bin.words = malloc(bin.size * sizeof(uint32_t));
    glslang_program_SPIRV_get(program, bin.words);

    const char* spirv_messages = glslang_program_SPIRV_get_messages(program);
    if (spirv_messages)
    {
        log_info("(%s) %s\b", filename, spirv_messages);
    }

    glslang_program_delete(program);
    glslang_shader_delete(shader);

    return bin;
}

// ==================== Shader =========================

string_t shader_stage_to_string(enum ShaderStage stage, arena_t* arena)
{
    string_t result;
    switch (stage)
    {
        case RPE_BACKEND_SHADER_STAGE_VERTEX:
            result = string_init("Vertex", arena);
            break;
        case RPE_BACKEND_SHADER_STAGE_FRAGMENT:
            result = string_init("Fragment", arena);
            break;
        case RPE_BACKEND_SHADER_STAGE_TESSE_CON:
            result = string_init("TesselationCon", arena);
            break;
        case RPE_BACKEND_SHADER_STAGE_TESSE_EVAL:
            result = string_init("TesselationEval", arena);
            break;
        case RPE_BACKEND_SHADER_STAGE_GEOM:
            result = string_init("Geometry", arena);
            break;
        case RPE_BACKEND_SHADER_STAGE_COMPUTE:
            result = string_init("Compute", arena);
            break;
    }
    return result;
}

VkShaderStageFlagBits shader_vk_stage_flag(enum ShaderStage stage)
{
    VkShaderStageFlagBits ret;
    switch (stage)
    {
        case RPE_BACKEND_SHADER_STAGE_VERTEX:
            ret = VK_SHADER_STAGE_VERTEX_BIT;
            break;
        case RPE_BACKEND_SHADER_STAGE_FRAGMENT:
            ret = VK_SHADER_STAGE_FRAGMENT_BIT;
            break;
        case RPE_BACKEND_SHADER_STAGE_TESSE_CON:
            ret = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
            break;
        case RPE_BACKEND_SHADER_STAGE_TESSE_EVAL:
            ret = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            break;
        case RPE_BACKEND_SHADER_STAGE_GEOM:
            ret = VK_SHADER_STAGE_GEOMETRY_BIT;
            break;
        case RPE_BACKEND_SHADER_STAGE_COMPUTE:
            ret = VK_SHADER_STAGE_COMPUTE_BIT;
            break;
    }
    return ret;
}

VkFormat shader_format_from_width(uint32_t width, uint32_t vecSize, spvc_basetype base_type)
{
    VkFormat format = VK_FORMAT_UNDEFINED;

    // floats
    if (base_type == SPVC_BASETYPE_FP32)
    {
        if (width == 32)
        {
            if (vecSize == 1)
            {
                format = VK_FORMAT_R32_SFLOAT;
            }
            if (vecSize == 2)
            {
                format = VK_FORMAT_R32G32_SFLOAT;
            }
            if (vecSize == 3)
            {
                format = VK_FORMAT_R32G32B32_SFLOAT;
            }
            if (vecSize == 4)
            {
                format = VK_FORMAT_R32G32B32A32_SFLOAT;
            }
        }
    }

    // signed integers
    else if (base_type == SPVC_BASETYPE_INT32)
    {
        if (width == 32)
        {
            if (vecSize == 1)
            {
                format = VK_FORMAT_R32_SINT;
            }
            if (vecSize == 2)
            {
                format = VK_FORMAT_R32G32_SINT;
            }
            if (vecSize == 3)
            {
                format = VK_FORMAT_R32G32B32_SINT;
            }
            if (vecSize == 4)
            {
                format = VK_FORMAT_R32G32B32A32_SINT;
            }
        }
    }
    return format;
}

uint32_t shader_stride_from_vec_size(uint32_t width, uint32_t vecSize, spvc_basetype base_type)
{
    uint32_t size = 0;

    // Floating 32-bit or 32-bit unsigned integers.
    if (base_type == SPVC_BASETYPE_FP32 || base_type == SPVC_BASETYPE_INT32)
    {
        if (width == 32)
        {
            if (vecSize == 1)
            {
                size = 4;
            }
            else if (vecSize == 2)
            {
                size = 8;
            }
            else if (vecSize == 3)
            {
                size = 12;
            }
            else if (vecSize == 4)
            {
                size = 16;
            }
        }
    }

    return size;
}

shader_t* shader_init(enum ShaderStage stage, arena_t* arena)
{
    shader_t* out = ARENA_MAKE_STRUCT(arena, shader_t, ARENA_ZERO_MEMORY);
    out->stage = stage;
    return out;
}

spirv_binary_t shader_load_spirv(const char* filename, arena_t* arena)
{
    spirv_binary_t bin = {.words = NULL, .size = 0};
    string_t shader_dir = string_init(RPE_SHADER_DIRECTORY, arena);
    string_t shader_filename = string_append(&shader_dir, filename, arena);
    FILE* fp = fopen(shader_filename.data, "rb");
    if (!fp)
    {
        log_error("Invalid shader SPIRV file: %s", shader_filename.data);
        return bin;
    }
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (bin.size <= 0)
    {
        log_error("Shader SPIRV file is empty: %s", shader_filename.data);
        return bin;
    }
    bin.size = file_size / sizeof(uint32_t);
    bin.words = ARENA_MAKE_ZERO_ARRAY(arena, uint32_t, bin.size);
    if (fread(bin.words, bin.size, sizeof(uint32_t), fp) != bin.size)
    {
        log_error("Error reading shader SPRIV: %s", shader_filename.data);
    }
    return bin;
}

spirv_binary_t
shader_compile(shader_t* shader, const char* shader_code, const char* filename, arena_t* arena)
{
    assert(shader);

    spirv_binary_t bin = {.size = 0, .words = NULL};
    if (!shader_code)
    {
        log_error("There is no shader code to process!");
        return bin;
    }
    // Compile into bytecode ready for wrapping.
    bin = shader_compiler_compile(
        shader_to_glslang_type(shader->stage), shader_code, filename, arena);
    return bin;
}

void shader_create_vk_module(shader_t* shader, vkapi_context_t* context, spirv_binary_t bin)
{
    assert(shader);

    // create the shader module
    VkShaderModuleCreateInfo shader_info = {0};
    shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_info.flags = 0;
    shader_info.codeSize = bin.size * sizeof(uint32_t);
    shader_info.pCode = bin.words;
    VK_CHECK_RESULT(
        vkCreateShaderModule(context->device, &shader_info, VK_NULL_HANDLE, &shader->module));

    // Create the wrapper - this will be used by the pipeline.
    memset(&shader->create_info, 0, sizeof(VkPipelineShaderStageCreateInfo));
    shader->create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader->create_info.stage = shader_vk_stage_flag(shader->stage);
    shader->create_info.module = shader->module;
    shader->create_info.pName = "main";
}

void _error_callback(void* data, const char* msg)
{
    log_error("Error during shader reflection: %s", msg);
}

void shader_reflect_spirv(shader_t* shader, uint32_t* spirv, uint32_t word_count, arena_t* arena)
{
    spvc_context context = NULL;
    spvc_parsed_ir ir = NULL;
    spvc_compiler compiler_glsl = NULL;
    spvc_resources resources = NULL;

    spvc_context_create(&context);
    spvc_context_set_error_callback(context, _error_callback, NULL);
    spvc_context_parse_spirv(context, spirv, word_count, &ir);
    spvc_context_create_compiler(
        context, SPVC_BACKEND_GLSL, ir, SPVC_CAPTURE_MODE_TAKE_OWNERSHIP, &compiler_glsl);

    const spvc_reflected_resource* list = NULL;
    size_t count = 0;

    spvc_compiler_create_shader_resources(compiler_glsl, &resources);

    // Input attributes.
    spvc_resources_get_resource_list_for_type(
        resources, SPVC_RESOURCE_TYPE_STAGE_INPUT, &list, &count);
    for (size_t i = 0; i < count; ++i)
    {
        shader_attr_t* attr =
            &shader->resource_binding.stage_inputs[shader->resource_binding.stage_input_count++];
        attr->location =
            spvc_compiler_get_decoration(compiler_glsl, list[i].id, SpvDecorationLocation);
        spvc_type t = spvc_compiler_get_type_handle(compiler_glsl, list[i].base_type_id);
        attr->format = shader_format_from_width(
            spvc_type_get_bit_width(t), spvc_type_get_vector_size(t), spvc_type_get_basetype(t));
        attr->stride = shader_stride_from_vec_size(
            spvc_type_get_bit_width(t), spvc_type_get_vector_size(t), spvc_type_get_basetype(t));
    }

    // Output attributes.
    spvc_resources_get_resource_list_for_type(
        resources, SPVC_RESOURCE_TYPE_STAGE_OUTPUT, &list, &count);
    for (size_t i = 0; i < count; ++i)
    {
        shader_attr_t* attr =
            &shader->resource_binding.stage_outputs[shader->resource_binding.stage_output_count++];
        attr->location =
            spvc_compiler_get_decoration(compiler_glsl, list[i].id, SpvDecorationLocation);
        spvc_type t = spvc_compiler_get_type_handle(compiler_glsl, list[i].base_type_id);
        attr->format = shader_format_from_width(
            spvc_type_get_bit_width(t), spvc_type_get_vector_size(t), spvc_type_get_basetype(t));
        attr->stride = shader_stride_from_vec_size(
            spvc_type_get_bit_width(t), spvc_type_get_vector_size(t), spvc_type_get_basetype(t));
    }

    // Image samplers.
    spvc_resources_get_resource_list_for_type(
        resources, SPVC_RESOURCE_TYPE_SAMPLED_IMAGE, &list, &count);
    for (size_t i = 0; i < count; ++i)
    {
        shader_desc_layout_t* layout =
            &shader->resource_binding.desc_layouts[shader->resource_binding.desc_layout_count++];
        layout->binding =
            spvc_compiler_get_decoration(compiler_glsl, list[i].id, SpvDecorationBinding);
        layout->set =
            spvc_compiler_get_decoration(compiler_glsl, list[i].id, SpvDecorationDescriptorSet);
        assert(layout->set == VKAPI_PIPELINE_SAMPLER_SET_VALUE);
        layout->type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layout->name = string_init(list[i].name, arena);
        layout->stage = shader_vk_stage_flag(shader->stage);
    }

    // Storage images.
    spvc_resources_get_resource_list_for_type(
        resources, SPVC_RESOURCE_TYPE_STORAGE_IMAGE, &list, &count);
    for (size_t i = 0; i < count; ++i)
    {
        shader_desc_layout_t* layout =
            &shader->resource_binding.desc_layouts[shader->resource_binding.desc_layout_count++];
        layout->binding =
            spvc_compiler_get_decoration(compiler_glsl, list[i].id, SpvDecorationBinding);
        layout->set =
            spvc_compiler_get_decoration(compiler_glsl, list[i].id, SpvDecorationDescriptorSet);
        assert(layout->set == VKAPI_PIPELINE_STORAGE_IMAGE_SET_VALUE);
        layout->type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        layout->name = string_init(list[i].name, arena);
        layout->stage = shader_vk_stage_flag(shader->stage);
    }

    // Uniform buffers.
    spvc_resources_get_resource_list_for_type(
        resources, SPVC_RESOURCE_TYPE_UNIFORM_BUFFER, &list, &count);
    for (size_t i = 0; i < count; ++i)
    {
        shader_desc_layout_t* layout =
            &shader->resource_binding.desc_layouts[shader->resource_binding.desc_layout_count++];
        layout->binding =
            spvc_compiler_get_decoration(compiler_glsl, list[i].id, SpvDecorationBinding);
        layout->set =
            spvc_compiler_get_decoration(compiler_glsl, list[i].id, SpvDecorationDescriptorSet);
        assert(
            layout->set == VKAPI_PIPELINE_UBO_SET_VALUE ||
            layout->set == VKAPI_PIPELINE_UBO_DYN_SET_VALUE);
        layout->type = layout->set == VKAPI_PIPELINE_UBO_SET_VALUE
            ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
            : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        layout->name = string_init(list[i].name, arena);
        layout->stage = shader_vk_stage_flag(shader->stage);
        spvc_compiler_get_declared_struct_size(
            compiler_glsl,
            spvc_compiler_get_type_handle(compiler_glsl, list[i].base_type_id),
            &layout->range);
    }

    // Storage buffers. TODO: Add dynamic storage buffers.
    spvc_resources_get_resource_list_for_type(
        resources, SPVC_RESOURCE_TYPE_STORAGE_BUFFER, &list, &count);
    for (size_t i = 0; i < count; ++i)
    {
        shader_desc_layout_t* layout =
            &shader->resource_binding.desc_layouts[shader->resource_binding.desc_layout_count++];
        layout->binding =
            spvc_compiler_get_decoration(compiler_glsl, list[i].id, SpvDecorationBinding);
        layout->set =
            spvc_compiler_get_decoration(compiler_glsl, list[i].id, SpvDecorationDescriptorSet);
        assert(layout->set == VKAPI_PIPELINE_SSBO_SET_VALUE);
        layout->type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        layout->name = string_init(list[i].name, arena);
        layout->stage = shader_vk_stage_flag(shader->stage);
        spvc_compiler_get_declared_struct_size_runtime_array(
            compiler_glsl,
            spvc_compiler_get_type_handle(compiler_glsl, list[i].base_type_id),
            1,
            &layout->range);
    }

    // push blocks
    spvc_resources_get_resource_list_for_type(
        resources, SPVC_RESOURCE_TYPE_PUSH_CONSTANT, &list, &count);
    for (size_t i = 0; i < count; ++i)
    {
        const spvc_buffer_range** ranges = NULL;
        size_t range_count;
        spvc_compiler_get_active_buffer_ranges(compiler_glsl, list[i].id, ranges, &range_count);
        for (size_t j = 0; j < range_count; ++j)
        {
            shader->resource_binding.push_block_size += ranges[j]->range;
        }
    }

    // specialisation constants
    const spvc_specialization_constant* consts = NULL;
    spvc_compiler_get_specialization_constants(compiler_glsl, &consts, &count);
    for (size_t i = 0; i < count; ++i)
    {
        spvc_constant c = spvc_compiler_get_constant_handle(compiler_glsl, consts[i].id);
        spvc_type_id ti = spvc_constant_get_type(c);
        spvc_type t = spvc_compiler_get_type_handle(compiler_glsl, ti);
        shader->resource_binding.spec_consts[i].id = consts[i].constant_id;
        // spvc_compiler_get_declared_struct_size(compiler_glsl, t,
        // &shader->resource_binding.spec_consts[i].size);
        shader->resource_binding.spec_const_count = count;
    }

    spvc_context_destroy(context);
}
