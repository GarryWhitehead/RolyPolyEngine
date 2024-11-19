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

#include "render_queue.h"

#include "commands.h"

#include <utility/arena.h>
#include <utility/sort.h>
#include <vulkan-api/driver.h>

rpe_render_queue_t* rpe_render_queue_init(arena_t* arena)
{
    rpe_render_queue_t* q = ARENA_MAKE_ZERO_STRUCT(arena, rpe_render_queue_t);
    q->gbuffer_bucket = rpe_command_bucket_init(RPE_RENDER_QUEUE_GBUFFER_SIZE, arena);
    q->lighting_bucket = rpe_command_bucket_init(RPE_RENDER_QUEUE_LIGHTING_SIZE, arena);
    q->post_process_bucket = rpe_command_bucket_init(RPE_RENDER_QUEUE_POST_PROCESS_SIZE, arena);
    return q;
}

void rpe_render_queue_submit(rpe_render_queue_t* q, vkapi_driver_t* driver)
{
    assert(q);

    uint64_t* gbuffer_sorted_indices = ARENA_MAKE_ARRAY(&driver->_scratch_arena, uint64_t, q->gbuffer_bucket->curr_index, 0);
    uint64_t* light_sorted_indices =
        ARENA_MAKE_ARRAY(&driver->_scratch_arena, uint64_t, q->gbuffer_bucket->curr_index, 0);
    uint64_t* post_process_sorted_indices =
        ARENA_MAKE_ARRAY(&driver->_scratch_arena, uint64_t, q->gbuffer_bucket->curr_index, 0);

    // Sort the queues first based upon their keys.
    radix_sort(q->gbuffer_bucket->keys, q->gbuffer_bucket->curr_index, &driver->_scratch_arena, gbuffer_sorted_indices);
    radix_sort(
        q->lighting_bucket->keys,
        q->lighting_bucket->curr_index,
        &driver->_scratch_arena,
        light_sorted_indices);
    radix_sort(
        q->post_process_bucket->keys,
        q->post_process_bucket->curr_index,
        &driver->_scratch_arena,
        post_process_sorted_indices);

    rpe_command_bucket_submit(q->gbuffer_bucket, gbuffer_sorted_indices, driver);
    rpe_command_bucket_submit(q->lighting_bucket, light_sorted_indices, driver);
    rpe_command_bucket_submit(q->post_process_bucket, post_process_sorted_indices, driver);
}

void rpe_render_queue_clear(rpe_render_queue_t* q)
{
    rpe_command_bucket_reset(q->gbuffer_bucket);
    rpe_command_bucket_reset(q->post_process_bucket);
    rpe_command_bucket_reset(q->lighting_bucket);
}

uint64_t rpe_render_queue_create_sort_key(material_sort_key_t key, enum SortKeyType type)
{
    uint64_t sort_key;
    switch (type)
    {
        case MATERIAL_KEY_SORT_PROGRAM: {
            sort_key = ((uint64_t)key.view_layer << VIEW_LAYER_BIT_SHIFT) |
                ((uint64_t)(key.screen_layer & 0xfffff) << SCREEN_LAYER_BIT_SHIFT) |
                ((uint64_t)(key.depth & 0xffff) << DEPTH_BIT_SHIFT) |
                ((uint64_t)(key.program_id & 0xffff));
            break;
        }
        case MATERIAL_KEY_SORT_DEPTH: {
            sort_key = ((uint64_t)key.view_layer << VIEW_LAYER_BIT_SHIFT) |
                ((uint64_t)(key.screen_layer & 0xfffff) << SCREEN_LAYER_BIT_SHIFT) |
                ((uint64_t)(key.program_id & 0xff) << PROGRAM_BIT_SHIFT) |
                ((uint64_t)(key.depth & 0xff));
            break;
        }
    }
    return sort_key;
}
