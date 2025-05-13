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

#ifndef __APP_KTX_LOADER_H__
#define __APP_KTX_LOADER_H__

#include "gltf/gltf_asset.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <utility/arena.h>

typedef struct MappedTexture rpe_mapped_texture_t;
struct DecodeEntry;
struct Job;

bool gltf_ktx_loader_decode_image(
    void* data, size_t sz, rpe_mapped_texture_t* tex, image_free_func* free_func);

void gltf_ktx_loader_push_job(
    rpe_engine_t* engine, struct DecodeEntry* job_entry, struct Job* parent_job);

#endif