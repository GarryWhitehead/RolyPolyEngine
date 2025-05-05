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
#include "backend/convert_to_vk.h"
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

    // Default rasterisation settings.
    out->raster_state.polygon_mode = VK_POLYGON_MODE_FILL;
    out->render_prim.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    out->raster_state.front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    out->render_prim.prim_restart = VK_FALSE;
    out->ds_state.compare_op = VK_COMPARE_OP_LESS;

    return out;
}

void shader_bundle_add_desc_binding(
    shader_prog_bundle_t* bundle, uint32_t size, uint32_t binding, VkDescriptorType type)
{
    assert(bundle);
    // NOTE: The buffer is set by a call to the update function below.
    desc_bind_info_t info = {.binding = binding, .size = size, .type = type, .buffer = UINT32_MAX};
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
    // TODO: Can we use the sampled and storage image reflected info?
}

void shader_bundle_update_ubo_desc(
    shader_prog_bundle_t* bundle, uint32_t binding, buffer_handle_t buffer)
{
    assert(bundle);
    assert(binding < VKAPI_PIPELINE_MAX_UBO_BIND_COUNT);
    bundle->ubos[binding].buffer = buffer;
}

void shader_bundle_update_ssbo_desc(
    shader_prog_bundle_t* bundle, uint32_t binding, buffer_handle_t buffer, uint32_t count)
{
    assert(bundle);
    assert(binding < VKAPI_PIPELINE_MAX_SSBO_BIND_COUNT);
    assert(bundle->ssbos[binding].size > 0);
    bundle->ssbos[binding].buffer = buffer;
    bundle->ssbos[binding].size *= count;
}

void shader_bundle_add_image_sampler(
    shader_prog_bundle_t* bundle, vkapi_driver_t* driver, texture_handle_t handle, uint8_t binding)
{
    assert(binding < VKAPI_PIPELINE_MAX_SAMPLER_BIND_COUNT && "Binding of is out of bounds.");
    vkapi_texture_t* t = vkapi_res_cache_get_tex2d(driver->res_cache, handle);
    assert(t->sampler);
    bundle->image_samplers[binding].handle = handle;
    bundle->image_samplers[binding].sampler = t->sampler;
    bundle->use_bound_samplers = true;
}

void shader_bundle_add_storage_image(
    shader_prog_bundle_t* bundle, texture_handle_t handle, uint8_t binding)
{
    assert(binding < VKAPI_PIPELINE_MAX_STORAGE_IMAGE_BOUND_COUNT);
    bundle->storage_images[binding] = handle;
}

void shader_bundle_add_render_primitive(
    shader_prog_bundle_t* bundle, VkPrimitiveTopology topo, VkBool32 prim_restart)
{
    assert(bundle);
    bundle->render_prim.prim_restart = prim_restart;
    bundle->render_prim.topology = topo;
}

void shader_bundle_set_cull_mode(shader_prog_bundle_t* bundle, enum CullMode mode)
{
    assert(bundle);
    bundle->raster_state.cull_mode = cull_mode_to_vk(mode);
}

void shader_bundle_set_depth_read_write_state(shader_prog_bundle_t* bundle, bool test_state, bool write_state, enum CompareOp depth_op)
{
    assert(bundle);
    bundle->ds_state.test_enable = test_state;
    bundle->ds_state.write_enable = write_state;
    bundle->ds_state.compare_op = compare_op_to_vk(depth_op);
}

void shader_bundle_set_depth_clamp_state(shader_prog_bundle_t* bundle, bool state)
{
    assert(bundle);
    bundle->raster_state.depth_clamp_enable = state;
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
        "Specialisation constant for this stage has no entries. Upadte descriptors from reflection "
        "must be called before this function to fill in the reflection properties.");

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

void shader_bundle_add_vertex_input_binding(
    shader_prog_bundle_t* bundle,
    shader_handle_t handle,
    vkapi_driver_t* driver,
    uint32_t firstIndex,
    uint32_t lastIndex,
    uint32_t binding,
    VkVertexInputRate input_rate)
{
    assert(bundle);
    assert(firstIndex <= lastIndex);
    assert(lastIndex < VKAPI_PIPELINE_MAX_VERTEX_ATTR_COUNT);

    shader_t* shader = program_cache_get_shader(driver->prog_manager, handle);
    assert(lastIndex < shader->resource_binding.stage_input_count);

    uint32_t offset = 0;
    for (uint32_t i = firstIndex; i <= lastIndex; ++i)
    {
        shader_attr_t* attr = &shader->resource_binding.stage_inputs[i];
        VkVertexInputAttributeDescription ad = {
            .location = attr->location,
            .format = attr->format,
            .binding = binding,
            .offset = offset};
        bundle->vert_attrs[i] = ad;
        offset += attr->stride;
    }

    bundle->vert_bind_desc[binding].stride = offset;
    bundle->vert_bind_desc[binding].binding = binding;
    bundle->vert_bind_desc[binding].inputRate = input_rate;
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

    // Push constants.
    if (shader->resource_binding.push_block_size > 0)
    {
        shader_bundle_create_push_block(
            bundle, shader->resource_binding.push_block_size, shader->stage);
    }

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

    // Create the set layouts required for creating a pipeline layout.
    for (uint32_t i = 0; i < shader->resource_binding.desc_layout_count; ++i)
    {
        shader_desc_layout_t* l = &shader->resource_binding.desc_layouts[i];

        assert(l->set < VKAPI_PIPELINE_MAX_DESC_SET_COUNT);
        VkDescriptorSetLayoutBinding set_binding = {0};
        set_binding.binding = l->binding;
        set_binding.descriptorType = l->type;
        set_binding.descriptorCount =
            l->type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER && l->bindless_sampler
            ? VKAPI_PIPELINE_MAX_SAMPLER_BINDLESS_COUNT
            : 1;
        set_binding.stageFlags = l->stage;

        VkDescriptorSetLayoutBinding* slb = bundle->desc_bindings[l->set];

        if (bundle->desc_binding_counts[l->set] > 0)
        {
            // Do a check to see if a set already has the same binding already added - buffers can
            // be used across different shader stages, so just OR the stage flags.
            int bind_idx = INT32_MAX;
            for (int j = 0; j < bundle->desc_binding_counts[l->set]; ++j)
            {
                if (slb[j].binding == l->binding)
                {
                    bind_idx = j;
                    break;
                }
            }
            if (bind_idx != INT32_MAX)
            {
                assert(slb[bind_idx].descriptorType == l->type);
                slb[bind_idx].stageFlags |= l->stage;
                continue;
            }
        }
        assert(
            bundle->desc_binding_counts[l->set] < VKAPI_PIPELINE_MAX_DESC_SET_LAYOUT_BINDING_COUNT);
        slb[bundle->desc_binding_counts[l->set]++] = set_binding;
    }

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

    for (size_t i = 0; i < c->program_bundles.size; ++i)
    {
        shader_prog_bundle_t* b = DYN_ARRAY_GET_PTR(shader_prog_bundle_t, &c->program_bundles, i);
        for (size_t j = 0; j < VKAPI_PIPELINE_MAX_DESC_SET_COUNT; ++j)
        {
            if (b->desc_layouts[j])
            {
                vkDestroyDescriptorSetLayout(
                    driver->context->device, b->desc_layouts[j], VK_NULL_HANDLE);
            }
        }
    }
    dyn_array_clear(&c->program_bundles);
}

shader_t* program_cache_get_shader(program_cache_t* c, shader_handle_t handle)
{
    assert(c);
    assert(handle.id < c->shaders.size);
    return DYN_ARRAY_GET_PTR(shader_t, &c->shaders, handle.id);
}