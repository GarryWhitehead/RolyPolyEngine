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

#ifndef __UTILITY_PARALLEL_FOR_H__
#define __UTILITY_PARALLEL_FOR_H__

#include "job_queue.h"
#include "arena.h"

#include <stdint.h>

typedef void (*parallel_for_func_t)(uint32_t, uint32_t, void*);

struct SplitConfig
{
    uint32_t max_split;
    uint32_t min_count;
};

struct ParallelForData
{
    job_queue_t* jq;
    job_t* parent;
    uint32_t start;
    uint32_t count;
    uint32_t splits;
    parallel_for_func_t func;
    void* data;
    struct SplitConfig* cfg;
    arena_t* arena;
};

job_t* parallel_for(
    job_queue_t* jq,
    job_t* parent,
    uint32_t start,
    uint32_t count,
    parallel_for_func_t func,
    void* data,
    struct SplitConfig* cfg,
    arena_t* arena);

#endif