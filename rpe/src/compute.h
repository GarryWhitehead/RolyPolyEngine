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

#ifndef __RPE_COMPUTE_H__
#define __RPE_COMPUTE_H__

#include "backend/enums.h"

#include <vulkan-api/descriptor_cache.h>
#include <vulkan-api/program_manager.h>
#include <vulkan-api/resource_cache.h>

typedef struct Shader shader_t;
typedef struct ShaderProgramBundle shader_prog_bundle_t;

typedef struct Compute
{
    buffer_handle_t ssbos[VKAPI_PIPELINE_MAX_SSBO_BIND_COUNT];
    buffer_handle_t ubos[VKAPI_PIPELINE_MAX_UBO_BIND_COUNT];

    shader_prog_bundle_t* bundle;
    shader_handle_t shader;
} rpe_compute_t;

rpe_compute_t*
rpe_compute_init_from_file(vkapi_driver_t* driver, const char* filename, arena_t* arena);
rpe_compute_t*
rpe_compute_init_from_text(vkapi_driver_t* driver, const char* shader_code, arena_t* arena);

void rpe_compute_add_storage_image(rpe_compute_t* c, texture_handle_t h, uint32_t binding);
void rpe_compute_add_image_sampler(
    rpe_compute_t* c,
    vkapi_driver_t* driver,
    texture_handle_t h,
    uint32_t binding,
    sampler_params_t sampler_params);

buffer_handle_t rpe_compute_bind_ubo(rpe_compute_t* c, vkapi_driver_t* driver, uint32_t binding);
buffer_handle_t
rpe_compute_bind_ssbo(rpe_compute_t* c, vkapi_driver_t* driver, uint32_t binding, size_t count);

void rpe_compute_download_ssbo_to_host(
    rpe_compute_t* c, vkapi_driver_t* driver, uint32_t binding, size_t size, void* host_buffer);

#endif
