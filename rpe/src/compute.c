/* Copyright (c) 2022 Garry Whitehead
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

#include "compute.h"

#include "engine.h"

#include <utility/arena.h>
#include <vulkan-api/buffer.h>
#include <vulkan-api/common.h>
#include <vulkan-api/sampler_cache.h>
#include <vulkan-api/shader.h>

rpe_compute_t*
rpe_compute_init_from_file(vkapi_driver_t* driver, const char* filename, arena_t* arena)
{
    rpe_compute_t* i = ARENA_MAKE_ZERO_STRUCT(arena, rpe_compute_t);
    i->bundle = program_cache_create_program_bundle(driver->prog_manager, arena);

    i->shader = program_cache_from_spirv(
        driver->prog_manager, driver->context, filename, RPE_BACKEND_SHADER_STAGE_COMPUTE, arena);
    if (i->shader.id == UINT32_MAX)
    {
        return NULL;
    }
    shader_bundle_update_descs_from_reflection(i->bundle, driver, i->shader, arena);

    return i;
}

rpe_compute_t*
rpe_compute_init_from_text(vkapi_driver_t* driver, const char* shader_code, arena_t* arena)
{
    rpe_compute_t* i = ARENA_MAKE_ZERO_STRUCT(arena, rpe_compute_t);
    i->bundle = program_cache_create_program_bundle(driver->prog_manager, arena);

    i->shader = program_cache_compile_shader(
        driver->prog_manager,
        driver->context,
        shader_code,
        RPE_BACKEND_SHADER_STAGE_COMPUTE,
        arena);
    if (i->shader.id == UINT32_MAX)
    {
        return NULL;
    }
    shader_bundle_update_descs_from_reflection(i->bundle, driver, i->shader, arena);

    return i;
}

void rpe_compute_add_storage_image(rpe_compute_t* c, texture_handle_t h, uint32_t binding)
{
    assert(c);
    assert(vkapi_tex_handle_is_valid(h));
    assert(binding < VKAPI_PIPELINE_MAX_STORAGE_IMAGE_BOUND_COUNT);
    shader_bundle_add_storage_image(c->bundle, h, binding);
}

void rpe_compute_add_image_sampler(
    rpe_compute_t* c, vkapi_driver_t* driver, texture_handle_t h, uint32_t binding)
{
    shader_bundle_add_image_sampler(c->bundle, driver, h, binding);
}

buffer_handle_t rpe_compute_bind_ubo(rpe_compute_t* c, vkapi_driver_t* driver, uint32_t binding)
{
    assert(binding < VKAPI_PIPELINE_MAX_UBO_BIND_COUNT);

    uint32_t ubo_size = c->bundle->ubos[binding].size;
    c->ubos[binding] = vkapi_res_cache_create_ubo(driver->res_cache, driver, ubo_size);
    shader_bundle_update_ubo_desc(c->bundle, binding, c->ubos[binding]);
    return c->ubos[binding];
}

void rpe_compute_bind_ubo_buffer(rpe_compute_t* c, uint32_t binding, buffer_handle_t ubo)
{
    assert(binding < VKAPI_PIPELINE_MAX_UBO_BIND_COUNT);
    assert(vkapi_buffer_handle_is_valid(ubo));

    c->ubos[binding] = ubo;
    shader_bundle_update_ubo_desc(c->bundle, binding, c->ubos[binding]);
}

buffer_handle_t rpe_compute_bind_ssbo(
    rpe_compute_t* c,
    vkapi_driver_t* driver,
    uint32_t binding,
    size_t count,
    VkBufferUsageFlags usage_flags,
    enum BufferType type)
{
    assert(binding < VKAPI_PIPELINE_MAX_SSBO_BIND_COUNT);
    assert(count > 0);

    uint32_t ssbo_size = c->bundle->ssbos[binding].size * count;
    c->ssbos[binding] =
        vkapi_res_cache_create_ssbo(driver->res_cache, driver, ssbo_size, usage_flags, type);
    shader_bundle_update_ssbo_desc(c->bundle, binding, c->ssbos[binding], count);
    return c->ssbos[binding];
}

buffer_handle_t rpe_compute_bind_ssbo_gpu_only(
    rpe_compute_t* c,
    vkapi_driver_t* driver,
    uint32_t binding,
    size_t count,
    VkBufferUsageFlags usage_flags)
{
    return rpe_compute_bind_ssbo(c, driver, binding, count, usage_flags, VKAPI_BUFFER_GPU_ONLY);
}

buffer_handle_t rpe_compute_bind_ssbo_host_gpu(
    rpe_compute_t* c,
    vkapi_driver_t* driver,
    uint32_t binding,
    size_t count,
    VkBufferUsageFlags usage_flags)
{
    return rpe_compute_bind_ssbo(c, driver, binding, count, usage_flags, VKAPI_BUFFER_HOST_TO_GPU);
}

buffer_handle_t rpe_compute_bind_ssbo_gpu_host(
    rpe_compute_t* c,
    vkapi_driver_t* driver,
    uint32_t binding,
    size_t count,
    VkBufferUsageFlags usage_flags)
{
    return rpe_compute_bind_ssbo(c, driver, binding, count, usage_flags, VKAPI_BUFFER_GPU_TO_HOST);
}

void rpe_compute_download_ssbo_to_host(
    rpe_compute_t* c, vkapi_driver_t* driver, uint32_t binding, size_t size, void* host_buffer)
{
    assert(c);
    assert(binding < VKAPI_PIPELINE_MAX_SSBO_BIND_COUNT);
    vkapi_buffer_t* buffer = vkapi_res_cache_get_buffer(driver->res_cache, c->ssbos[binding]);
    assert(buffer);
    vkapi_buffer_download_to_host(buffer, driver, host_buffer, size);
}
