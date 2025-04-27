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
#ifndef __RPE_IBL_H__
#define __RPE_IBL_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct Ibl ibl_t;
typedef struct Engine rpe_engine_t;
typedef struct Scene rpe_scene_t;

struct PreFilterOptions
{
    int brdf_sample_count;
    int specular_sample_count;
    int specular_level_count;
};

ibl_t* rpe_ibl_init(rpe_engine_t* engine, rpe_scene_t* scene, struct PreFilterOptions options);

bool rpe_ibl_eqirect_to_cubemap(
    ibl_t* ibl, rpe_engine_t* engine, float* image_data, uint32_t width, uint32_t height);

bool rpe_ibl_upload_cubemap(
    ibl_t* ibl,
    rpe_engine_t* engine,
    const float* image_data,
    uint32_t image_sz,
    uint32_t width,
    uint32_t height,
    uint32_t mip_levels,
    size_t* mip_offsets);

bool rpe_ibl_create_env_maps(ibl_t* ibl, rpe_engine_t* engine);

#endif