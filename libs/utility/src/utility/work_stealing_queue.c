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

#include "work_stealing_queue.h"

#include "arena.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

void* _item_queue_ptr_offset(work_stealing_queue_t* q, ptrdiff_t offset)
{
    return q->items + (offset * q->type_size);
}

void _set_item(work_stealing_queue_t* queue, int idx, void* item)
{
    int mask_idx = idx & queue->idx_mask;
    memcpy(_item_queue_ptr_offset(queue, mask_idx), item, queue->type_size);
}

void* _get_item(work_stealing_queue_t* queue, int idx)
{
    int mask_idx = idx & queue->idx_mask;
    return _item_queue_ptr_offset(queue, mask_idx);
}

work_stealing_queue_t
work_stealing_queue_init(arena_t* arena, uint32_t queue_count, uint32_t type_size)
{
    assert(type_size > 0);
    assert(queue_count > 0);
    assert(arena);

    work_stealing_queue_t q;
    q.bottom_idx = 0;
    q.top_idx = 0;
    q.type_size = type_size;
    q.idx_mask = queue_count - 1;
    q.items = arena_alloc(arena, type_size, type_size, queue_count, ARENA_ZERO_MEMORY);
    return q;
}

void work_stealing_queue_push(work_stealing_queue_t* queue, void* item)
{
    int bottom = atomic_load_explicit(&queue->bottom_idx, memory_order_relaxed);
    _set_item(queue, bottom, item);
    atomic_store_explicit(&queue->bottom_idx, bottom + 1, memory_order_release);
}

void* work_stealing_queue_pop(work_stealing_queue_t* queue)
{
    atomic_int bottom = atomic_fetch_sub_explicit(&queue->bottom_idx, 1, memory_order_seq_cst) - 1;

    // Trying to pop from an empty queue?
    assert(bottom >= -1);

    // Store the top index - used to check for concurrent "steals" in case the queue is empty after
    // a pop.
    int top = atomic_load_explicit(&queue->top_idx, memory_order_seq_cst);

    if (top < bottom)
    {
        return _get_item(queue, bottom);
    }

    void* item = NULL;
    if (top == bottom)
    {
        item = _get_item(queue, bottom);

        if (atomic_compare_exchange_strong_explicit(
                &queue->top_idx, &top, top + 1, memory_order_seq_cst, memory_order_relaxed))
        {
            top++;
        }
        else
        {
            item = NULL;
        }
    }
    atomic_store_explicit(&queue->bottom_idx, top, memory_order_relaxed);
    return item;
}

void* work_stealing_queue_steal(work_stealing_queue_t* queue)
{
    // Keep spinning until we successfully manage to steal a job.
    while (true)
    {
        int top = atomic_load_explicit(&queue->top_idx, memory_order_seq_cst);
        int bottom = atomic_load_explicit(&queue->bottom_idx, memory_order_seq_cst);

        // The queue is empty.
        if (top >= bottom)
        {
            return NULL;
        }

        void* item = _get_item(queue, top);
        if (atomic_compare_exchange_strong_explicit(
                &queue->top_idx, &top, top + 1, memory_order_seq_cst, memory_order_relaxed))
        {
            return item;
        }
    }
}
