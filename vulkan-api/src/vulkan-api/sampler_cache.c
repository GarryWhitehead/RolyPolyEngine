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

#include "sampler_cache.h"

#include "backend/convert_to_vk.h"
#include "backend/enums.h"
#include "driver.h"

#include <utility/arena.h>
#include <utility/hash.h>

vkapi_sampler_cache_t* vkapi_sampler_cache_init(arena_t* arena)
{
    vkapi_sampler_cache_t* c = ARENA_MAKE_ZERO_STRUCT(arena, vkapi_sampler_cache_t);
    c->samplers = HASH_SET_CREATE(sampler_params_t, VkSampler*, arena, murmur_hash3);
    return c;
}

VkSampler* vkapi_sampler_cache_create(
    vkapi_sampler_cache_t* c, sampler_params_t* params, vkapi_context_t* context)
{
    VkSampler* s = HASH_SET_GET(&c->samplers, params);
    if (s)
    {
        return s;
    }

    VkSamplerCreateInfo ci = {};
    ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    ci.compareEnable = params->enable_compare;
    ci.anisotropyEnable = params->enable_anisotropy;
    ci.maxAnisotropy = params->anisotropy;
    ci.maxLod = params->mip_levels == 0 ? 0.25f : (float)params->mip_levels;
    ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    ci.unnormalizedCoordinates = VK_FALSE;
    ci.minFilter = sampler_filter_to_vk(params->min);
    ci.magFilter = sampler_filter_to_vk(params->mag);
    ci.addressModeU = sampler_addr_mode_to_vk(params->addr_u);
    ci.addressModeV = sampler_addr_mode_to_vk(params->addr_v);
    ci.addressModeW = sampler_addr_mode_to_vk(params->addr_w);
    ci.compareOp = compare_op_to_vk(params->compare_op);

    VkSampler sampler;
    VK_CHECK_RESULT(vkCreateSampler(context->device, &ci, VK_NULL_HANDLE, &sampler));

    return hash_set_insert(&c->samplers, params, &sampler);
}

void vkapi_sampler_cache_destroy(vkapi_sampler_cache_t* c, vkapi_driver_t* driver)
{
    hash_set_iterator_t it = hash_set_iter_create(&c->samplers);
    for (;;)
    {
        VkSampler* s = hash_set_iter_next(&it);
        if (!s)
        {
            break;
        }
        vkDestroySampler(driver->context->device, *s, VK_NULL_HANDLE);
    }
    hash_set_clear(&c->samplers);
}
