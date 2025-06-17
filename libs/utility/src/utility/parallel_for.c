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

#include "parallel_for.h"

#include <stdio.h>

#define MAX_SPLITS 12
#define MIN_COUNT 64

// Forward declaration.
void parallel_for_impl(
    job_queue_t* jq,
    job_t* parent,
    uint32_t start,
    uint32_t count,
    uint32_t splits,
    parallel_for_func_t func,
    void* data,
    struct SplitConfig* cfg,
    arena_t* arena);

bool should_split(uint32_t splits, uint32_t count, uint32_t max_split, uint32_t min_count)
{
    return splits < max_split && count >= min_count * 2;
}

void parallel_for_runner(void* data)
{
    assert(data);
    struct ParallelForData* d = (struct ParallelForData*)data;
    parallel_for_impl(
        d->jq, d->parent, d->start, d->count, d->splits, d->func, d->data, d->cfg, d->arena);
}

void parallel_for_impl(
    job_queue_t* jq,
    job_t* parent,
    uint32_t start,
    uint32_t count,
    uint32_t splits,
    parallel_for_func_t func,
    void* func_data,
    struct SplitConfig* cfg,
    arena_t* arena)
{
    uint32_t max_split = !cfg ? MAX_SPLITS : cfg->max_split;
    uint32_t min_count = !cfg ? MIN_COUNT : cfg->min_count;

    if (should_split(splits, count, max_split, min_count))
    {
        // left side
        uint32_t left_count = count / 2;

        struct ParallelForData* l_data =
            ARENA_MAKE_ZERO_STRUCT_WITH_LOCK(arena, struct ParallelForData);
        l_data->func = func;
        l_data->count = left_count;
        l_data->start = start;
        l_data->splits = ++splits;
        l_data->parent = parent;
        l_data->data = func_data;
        l_data->cfg = cfg;
        l_data->jq = jq;
        l_data->arena = arena;
        job_t* left = job_queue_create_job(jq, parallel_for_runner, l_data, parent);
        job_queue_run_job(jq, left);

        // right side
        uint32_t right_count = count - left_count;

        struct ParallelForData* r_data =
            ARENA_MAKE_ZERO_STRUCT_WITH_LOCK(arena, struct ParallelForData);
        r_data->func = func;
        r_data->count = right_count;
        r_data->start = start + left_count;
        r_data->splits = splits;
        r_data->parent = parent;
        r_data->data = func_data;
        r_data->cfg = cfg;
        r_data->jq = jq;
        r_data->arena = arena;

        job_t* right = job_queue_create_job(jq, parallel_for_runner, r_data, parent);
        job_queue_run_job(jq, right);
    }
    else
    {
        func(start, count, func_data);
    }
}

job_t* parallel_for(
    job_queue_t* jq,
    job_t* parent,
    uint32_t start,
    uint32_t count,
    parallel_for_func_t func,
    void* data,
    struct SplitConfig* cfg,
    arena_t* arena)
{
    struct ParallelForData* p_data = ARENA_MAKE_ZERO_STRUCT(arena, struct ParallelForData);
    p_data->func = func;
    p_data->count = count;
    p_data->start = start;
    p_data->splits = 0;
    p_data->parent = parent;
    p_data->data = data;
    p_data->cfg = cfg;
    p_data->jq = jq;
    p_data->arena = arena;

    return job_queue_create_job(jq, parallel_for_runner, p_data, parent);
}