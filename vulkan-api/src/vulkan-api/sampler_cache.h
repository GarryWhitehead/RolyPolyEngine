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

#ifndef __VKAPI_SAMPLER_CACHE_H__
#define __VKAPI_SAMPLER_CACHE_H__

#include "backend/enums.h"
#include "common.h"

#include <utility/hash_set.h>

typedef struct VkApiDriver vkapi_driver_t;
typedef struct VkApiContext vkapi_context_t;

/**
 Keep track of samplers dispersed between textures
 with all samplers kept track of in one place.
 */
typedef struct SamplerCache
{
    hash_set_t samplers;
} vkapi_sampler_cache_t;

vkapi_sampler_cache_t* vkapi_sampler_cache_init(arena_t* arena);

VkSampler* vkapi_sampler_cache_create(
    vkapi_sampler_cache_t* c, sampler_params_t* params, vkapi_context_t* context);

void vkapi_sampler_cache_destroy(vkapi_sampler_cache_t* c, vkapi_driver_t* driver);

#endif
