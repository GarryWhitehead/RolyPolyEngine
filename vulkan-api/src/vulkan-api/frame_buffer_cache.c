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

#include "frame_buffer_cache.h"

#include "driver.h"
#include "pipeline.h"
#include "renderpass.h"

vkapi_rpass_key_t vkapi_rpass_key_init()
{
    vkapi_rpass_key_t i;
    memset(&i, 0, sizeof(vkapi_rpass_key_t));
    return i;
}

vkapi_fbo_key_t vkapi_fbo_key_init()
{
    vkapi_fbo_key_t i;
    memset(&i, 0, sizeof(vkapi_fbo_key_t));
    return i;
}

vkapi_fb_cache_t* vkapi_fb_cache_init(arena_t* arena)
{
    vkapi_fb_cache_t* i = ARENA_MAKE_ZERO_STRUCT(arena, vkapi_fb_cache_t);
    i->render_passes = HASH_SET_CREATE(vkapi_rpass_key_t, vkapi_rpass_t, arena, murmur_hash3);
    i->fbos = HASH_SET_CREATE(vkapi_fbo_key_t, vkapi_fbo_t, arena, murmur_hash3);
    return i;
}

vkapi_rpass_t* vkapi_fb_cache_find_or_create_rpass(
    vkapi_fb_cache_t* cache, vkapi_rpass_key_t* key, vkapi_driver_t* driver, arena_t* arena)
{
    assert(cache);
    assert(driver);

    vkapi_rpass_t* rpass = HASH_SET_GET(&cache->render_passes, key);
    if (rpass)
    {
        rpass->last_used_frame_stamp = driver->current_frame;
        return rpass;
    }

    // create a new renderpass
    vkapi_rpass_t new_rpass = vkapi_rpass_init(arena);
    new_rpass.last_used_frame_stamp = driver->current_frame;

    // add the colour attachments
    for (int idx = 0; idx < VKAPI_RENDER_TARGET_MAX_COLOR_ATTACH_COUNT; ++idx)
    {
        if (key->colour_formats[idx] != VK_FORMAT_UNDEFINED)
        {
            assert(key->final_layout[idx] != VK_IMAGE_LAYOUT_UNDEFINED);
            struct VkApiAttachment attach;
            attach.format = key->colour_formats[idx];
            attach.initial_layout = key->initial_layout[idx];
            attach.final_layout = key->final_layout[idx];
            attach.load_op = key->load_op[idx];
            attach.store_op = key->store_op[idx];
            attach.stencil_load_op = key->ds_load_op[1];
            attach.stencil_store_op = key->ds_store_op[1];
            vkapi_rpass_add_attach(&new_rpass, &attach);
        }
    }
    if (key->depth != VK_FORMAT_UNDEFINED)
    {
        struct VkApiAttachment attach;
        attach.format = key->depth;
        attach.initial_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        attach.final_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        attach.load_op = key->ds_load_op[0];
        attach.store_op = key->ds_store_op[0];
        attach.stencil_load_op = key->ds_load_op[1];
        attach.stencil_store_op = key->ds_store_op[1];
        vkapi_rpass_add_attach(&new_rpass, &attach);
    }
    vkapi_rpass_create(&new_rpass, driver, key->multi_view);

    return HASH_SET_INSERT(&cache->render_passes, key, &new_rpass);
}

vkapi_fbo_t* vkapi_fb_cache_find_or_create_fbo(
    vkapi_fb_cache_t* cache, vkapi_fbo_key_t* key, uint32_t count, vkapi_driver_t* driver)
{
    assert(cache);
    vkapi_fbo_t* fbo = HASH_SET_GET(&cache->fbos, key);
    if (fbo)
    {
        return fbo;
    }

    vkapi_fbo_t new_fbo = vkapi_fbo_init();
    vkapi_fbo_create(
        &new_fbo, driver, key->renderpass, key->views, count, key->width, key->height, key->layer);
    return HASH_SET_INSERT(&cache->fbos, key, &new_fbo);
}

void vkapi_fb_cache_gc(vkapi_fb_cache_t* c, vkapi_driver_t* driver, uint64_t current_frame)
{
    assert(c);
    hash_set_iterator_t it = hash_set_iter_create(&c->fbos);
    for (;;)
    {
        vkapi_fbo_t* fbo = hash_set_iter_next(&it);
        if (!fbo)
        {
            break;
        }

        uint64_t frames_until_collection =
            fbo->last_used_frame_stamp + VKAPI_PIPELINE_LIFETIME_FRAME_COUNT;
        if (frames_until_collection < current_frame)
        {
            vkDestroyFramebuffer(driver->context->device, fbo->instance, VK_NULL_HANDLE);
            it = hash_set_iter_erase(&it);
        }
    }
    it = hash_set_iter_create(&c->render_passes);
    for (;;)
    {
        vkapi_rpass_t* rpass = hash_set_iter_next(&it);
        if (!rpass)
        {
            break;
        }

        uint64_t frames_until_collection =
            rpass->last_used_frame_stamp + VKAPI_PIPELINE_LIFETIME_FRAME_COUNT;
        if (frames_until_collection < current_frame)
        {
            vkDestroyRenderPass(driver->context->device, rpass->instance, VK_NULL_HANDLE);
            it = hash_set_iter_erase(&it);
        }
    }
}

void vkapi_fb_cache_destroy(vkapi_fb_cache_t* c, vkapi_driver_t* driver)
{
    assert(c);
    hash_set_iterator_t it = hash_set_iter_create(&c->fbos);
    for (;;)
    {
        vkapi_fbo_t* fbo = hash_set_iter_next(&it);
        if (!fbo)
        {
            break;
        }
        vkDestroyFramebuffer(driver->context->device, fbo->instance, VK_NULL_HANDLE);
    }
    hash_set_clear(&c->fbos);

    it = hash_set_iter_create(&c->render_passes);
    for (;;)
    {
        vkapi_rpass_t* rpass = hash_set_iter_next(&it);
        if (!rpass)
        {
            break;
        }
        vkDestroyRenderPass(driver->context->device, rpass->instance, VK_NULL_HANDLE);
    }
    hash_set_clear(&c->render_passes);
}