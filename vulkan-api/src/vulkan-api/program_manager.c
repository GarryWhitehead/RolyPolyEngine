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

#include "program_manager.h"

#include "backend/enums.h"
#include "driver.h"
#include "pipeline_cache.h"
#include "shader.h"

#include <assert.h>
#include <utility/filesystem.h>
#include <utility/hash.h>

bool vkapi_is_valid_shader_handle(shader_handle_t h) { return h.id != VKAPI_INVALID_SHADER_HANDLE; }

void vkapi_invalidate_shader_handle(shader_handle_t* h) { h->id = VKAPI_INVALID_SHADER_HANDLE; }

shader_prog_bundle_t* shader_bundle_init(arena_t* arena)
{
    shader_prog_bundle_t* out = ARENA_MAKE_STRUCT(arena, shader_prog_bundle_t, ARENA_ZERO_MEMORY);
    for (int i = 0; i < VKAPI_PIPELINE_MAX_STORAGE_IMAGE_BOUND_COUNT; ++i)
    {
        vkapi_invalidate_tex_handle(&out->storage_images[i]);
    }
    for (int i = 0; i < VKAPI_PIPELINE_MAX_SAMPLER_BIND_COUNT; ++i)
    {
        vkapi_invalidate_tex_handle(&out->image_samplers[i].handle);
    }
    for (int i = 0; i < VKAPI_PIPELINE_MAX_UBO_BIND_COUNT; ++i)
    {
        vkapi_invalidate_buffer_handle(&out->ubos[i].buffer);
    }
    for (int i = 0; i < VKAPI_PIPELINE_MAX_SSBO_BIND_COUNT; ++i)
    {
        vkapi_invalidate_buffer_handle(&out->ssbos[i].buffer);
    }
    for (int i = 0; i < RPE_BACKEND_SHADER_STAGE_MAX_COUNT; ++i)
    {
        vkapi_invalidate_shader_handle(&out->shaders[i]);
    }
    return out;
}

void shader_bundle_add_desc_binding(
    shader_prog_bundle_t* bundle, uint32_t size, uint32_t binding, VkDescriptorType type)
{
    assert(bundle);
    assert(size > 0);
    assert(type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    desc_bind_info_t info = {.binding = binding, .size = size, .type = type};
    if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
    {
        assert(binding < VKAPI_PIPELINE_MAX_UBO_BIND_COUNT);
        bundle->ubos[binding] = info;
    }
    else if (type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
    {
        assert(binding < VKAPI_PIPELINE_MAX_SSBO_BIND_COUNT);
        bundle->ssbos[binding] = info;
    }
}

void shader_bundle_update_desc_buffer(
    shader_prog_bundle_t* bundle, uint32_t binding, VkDescriptorType type, buffer_handle_t buffer)
{
    assert(bundle);
    if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
    {
        assert(binding < VKAPI_PIPELINE_MAX_UBO_BIND_COUNT);
        bundle->ubos[binding].buffer = buffer;
    }
    else if (type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
    {
        assert(binding < VKAPI_PIPELINE_MAX_SSBO_BIND_COUNT);
        bundle->ssbos[binding].buffer = buffer;
    }
}

void shader_bundle_add_image_sampler(
    shader_prog_bundle_t* bundle, texture_handle_t handle, uint8_t binding, VkSampler sampler)
{
    assert(binding < VKAPI_PIPELINE_MAX_SAMPLER_BIND_COUNT && "Binding of is out of bounds.");
    bundle->image_samplers[binding].handle = handle;
    bundle->image_samplers[binding].sampler = sampler;
}

void shader_bundle_add_storage_image(
    shader_prog_bundle_t* bundle, texture_handle_t handle, uint8_t binding)
{
    assert(binding < VKAPI_PIPELINE_MAX_STORAGE_IMAGE_BOUND_COUNT);
    bundle->storage_images[binding] = handle;
}

void shader_bundle_set_push_block_data(
    shader_prog_bundle_t* bundle, enum ShaderStage stage, void* data)
{
    assert(data);
    bundle->push_blocks[stage].data = data;
}

void shader_bundle_add_render_primitive(
    shader_prog_bundle_t* bundle, VkPrimitiveTopology topo, VkBool32 prim_restart)
{
    assert(bundle);
    bundle->render_prim.prim_restart = prim_restart;
    bundle->render_prim.topology = topo;
}

void shader_bundle_set_scissor(
    shader_prog_bundle_t* bundle,
    uint32_t width,
    uint32_t height,
    uint32_t x_offset,
    uint32_t y_offset)
{
    assert(bundle);
    VkRect2D rect = {
        .offset = {.x = (int32_t)x_offset, .y = (int32_t)y_offset},
        .extent = {.width = width, .height = height}};
    bundle->scissor = rect;
}

void shader_bundle_set_viewport(
    shader_prog_bundle_t* bundle, uint32_t width, uint32_t height, float minDepth, float maxDepth)
{
    assert(bundle);
    assert(width > 0);
    assert(height > 0);

    VkViewport vp = {
        .width = (float)width,
        .height = (float)height,
        .x = 0,
        .y = 0,
        .minDepth = minDepth,
        .maxDepth = maxDepth,
    };
    bundle->viewport = vp;
}

void shader_bundle_add_texture_sampler(
    shader_prog_bundle_t* bundle, VkSampler sampler, uint32_t binding)
{
    assert(bundle);
    assert(binding < VKAPI_PIPELINE_MAX_SAMPLER_BIND_COUNT);
    bundle->image_samplers[binding].sampler = sampler;
}

void shader_bundle_create_push_block(
    shader_prog_bundle_t* bundle, size_t size, enum ShaderStage stage)
{
    assert(bundle);
    assert(size > 0);
    bundle->push_blocks[stage].stage = shader_vk_stage_flag(stage);
    bundle->push_blocks[stage].range = size;
}

void shader_bundle_update_spec_const_data(
    shader_prog_bundle_t* bundle, uint32_t data_size, void* data, enum ShaderStage stage)
{
    assert(bundle);
    assert(data);
    assert(
        bundle->spec_const_params[stage].entry_count > 0 &&
        "Specialisation constant for this stage has no entries.");

    bundle->spec_const_params[stage].data_size = data_size;
    bundle->spec_const_params[stage].data = data;
}

void shader_bundle_get_shader_stage_create_info_all(
    shader_prog_bundle_t* b, vkapi_driver_t* driver, VkPipelineShaderStageCreateInfo* out)
{
    for (int i = 0; i < RPE_BACKEND_SHADER_STAGE_MAX_COUNT; ++i)
    {
        if (vkapi_is_valid_shader_handle(b->shaders[i]))
        {
            shader_t* shader = program_cache_get_shader(driver->prog_manager, b->shaders[i]);
            out[i] = shader->create_info;
        }
    }
}

VkPipelineShaderStageCreateInfo shader_bundle_get_shader_stage_create_info(
    shader_prog_bundle_t* b, vkapi_driver_t* driver, enum ShaderStage stage)
{
    assert(vkapi_is_valid_shader_handle(b->shaders[stage]));
    shader_t* shader = program_cache_get_shader(driver->prog_manager, b->shaders[stage]);
    return shader->create_info;
}

void shader_bundle_update_descs_from_reflection(
    shader_prog_bundle_t* bundle, vkapi_driver_t* driver, shader_handle_t handle, arena_t* arena)
{
    assert(bundle);
    shader_t* shader = program_cache_get_shader(driver->prog_manager, handle);

    // Buffer bindings.
    for (uint32_t i = 0; i < shader->resource_binding.desc_layout_count; ++i)
    {
        shader_desc_layout_t* l = &shader->resource_binding.desc_layouts[i];
        shader_bundle_add_desc_binding(bundle, l->range, l->binding, l->type);
    }

    // Vertex input attributes.
    uint32_t input_stride = 0;
    for (uint32_t i = 0; i < shader->resource_binding.stage_input_count; ++i)
    {
        shader_attr_t* attr = &shader->resource_binding.stage_inputs[i];
        VkVertexInputAttributeDescription ad = {
            .location = attr->location,
            .format = attr->format,
            .binding = 0,
            .offset = attr->stride};
        bundle->vert_attrs[i] = ad;
        input_stride += attr->stride;
    }
    bundle->vert_bind_desc.stride = input_stride;
    bundle->vert_bind_desc.binding = 0;
    bundle->vert_bind_desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // Specialization constants.
    uint32_t offset = 0;
    bundle->spec_const_params[shader->stage].entry_count =
        shader->resource_binding.spec_const_count;
    for (uint32_t i = 0; i < shader->resource_binding.spec_const_count; ++i)
    {
        struct SpecializationConst* spec_const = &shader->resource_binding.spec_consts[i];
        VkSpecializationMapEntry entry = {
            .constantID = spec_const->id, .size = spec_const->size, .offset = offset};
        offset += entry.size;
        bundle->spec_const_params[shader->stage].entries[i] = entry;
    }

    // Create the pipeline layouts.
    vkapi_desc_cache_create_layouts(shader, driver, bundle, arena);

    bundle->shaders[shader->stage] = handle;
}

program_cache_t* program_cache_init(arena_t* arena)
{
    program_cache_t* out = ARENA_MAKE_STRUCT(arena, program_cache_t, ARENA_ZERO_MEMORY);
    MAKE_DYN_ARRAY(shader_prog_bundle_t, arena, 50, &out->program_bundles);
    MAKE_DYN_ARRAY(shader_t, arena, 50, &out->shaders);
    return out;
}

shader_handle_t program_cache_compile_shader(
    program_cache_t* c,
    vkapi_context_t* context,
    const char* shader_code,
    enum ShaderStage stage,
    arena_t* arena)
{
    shader_handle_t h;
    shader_t* shader = shader_init(stage, arena);
    spirv_binary_t bin = shader_compile(shader, shader_code, "", arena);
    if (!bin.words)
    {
        h.id = UINT32_MAX;
        return h;
    }
    shader_reflect_spirv(shader, bin.words, bin.size, arena);
    shader_create_vk_module(shader, context, bin);

    h.id = c->shaders.size;
    DYN_ARRAY_APPEND(&c->shaders, shader);
    return h;
}

shader_handle_t program_cache_from_spirv(
    program_cache_t* c,
    vkapi_context_t* context,
    const char* filename,
    enum ShaderStage stage,
    arena_t* arena)
{
    shader_handle_t h;
    shader_t* shader = shader_init(stage, arena);
    spirv_binary_t bin = shader_load_spirv(filename, arena);
    if (!bin.words)
    {
        h.id = UINT32_MAX;
        return h;
    }
    shader_reflect_spirv(shader, bin.words, bin.size, arena);
    shader_create_vk_module(shader, context, bin);

    h.id = c->shaders.size;
    DYN_ARRAY_APPEND(&c->shaders, shader);
    return h;
}

shader_prog_bundle_t* program_cache_create_program_bundle(program_cache_t* c, arena_t* arena)
{
    assert(c);
    shader_prog_bundle_t* b = shader_bundle_init(arena);
    return DYN_ARRAY_APPEND(&c->program_bundles, b);
}

void program_cache_destroy(program_cache_t* c, vkapi_driver_t* driver)
{
    for (size_t i = 0; i < c->shaders.size; ++i)
    {
        shader_t* s = DYN_ARRAY_GET_PTR(shader_t, &c->shaders, i);
        vkDestroyShaderModule(driver->context->device, s->module, VK_NULL_HANDLE);
    }
    dyn_array_clear(&c->shaders);
}

shader_t* program_cache_get_shader(program_cache_t* c, shader_handle_t handle)
{
    assert(c);
    assert(handle.id < c->shaders.size);
    return DYN_ARRAY_GET_PTR(shader_t, &c->shaders, handle.id);
}