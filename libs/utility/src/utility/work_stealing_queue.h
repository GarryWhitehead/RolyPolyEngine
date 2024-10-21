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
#ifndef __UTILITY_WORK_STEALING_QUEUE_H__
#define __UTILITY_WORK_STEALING_QUEUE_H__

#include <stdint.h>
#include <stdatomic.h>

// Forward declarations.
typedef struct Arena arena_t;

typedef struct WorkStealingDeque
{
    atomic_int top_idx;
    atomic_int bottom_idx;
    int idx_mask;
    uint32_t type_size;
    void* items;

} work_stealing_queue_t;

#define WORK_STEALING_DEQUE_INIT(type, arena, count)                                               \
    work_stealing_queue_init(arena, count, sizeof(type));

#ifdef __linux__
#define WORK_STEALING_QUEUE_PUSH(queue, item)                                                      \
    ({                                                                                             \
        __auto_type _item = item;                                                                  \
        assert(sizeof(*_item) == (queue)->type_size);                                              \
        work_stealing_queue_push(queue, _item);                                                    \
    })
#else
/*                                                                                             \
#define WORK_STEALING_QUEUE_PUSH(queue, item)                                                      \
    {                                                                                             \
        auto _item = item;                                                                  \
        assert(sizeof(*_item) == (queue)->type_size);                                              \
        work_stealing_queue_push(queue, _item);                                                    \
    }*/
#define WORK_STEALING_QUEUE_PUSH(queue, item) work_stealing_queue_push(queue, item);
    
#endif

work_stealing_queue_t
work_stealing_queue_init(arena_t* arena, uint32_t queue_count, uint32_t type_size);

void work_stealing_queue_push(work_stealing_queue_t* queue, void* item);

void* work_stealing_queue_pop(work_stealing_queue_t* queue);

void* work_stealing_queue_steal(work_stealing_queue_t* queue);

#endif