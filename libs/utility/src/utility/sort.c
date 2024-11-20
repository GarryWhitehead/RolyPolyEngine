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

#include "sort.h"

#include "array_utility.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

void count_sort(uint64_t* arr, size_t sz, int pos, arena_t* arena, uint64_t* output)
{
    uint64_t bucket[10];
    memset(bucket, 0, sizeof(uint64_t) * 10);

    uint64_t* sorted = ARENA_MAKE_ARRAY(arena, uint64_t, sz, 0);
    uint64_t* tmp = ARENA_MAKE_ARRAY(arena, uint64_t, sz, 0);

    for (size_t i = 0; i < sz; ++i)
    {
        ++bucket[(arr[i] / pos) % 10];
    }
    for (int i = 1; i < 10; ++i)
    {
        bucket[i] += bucket[i - 1];
    }
    for (int i = (int)sz - 1; i >= 0; i--)
    {
        uint64_t index = (arr[i] / pos) % 10;
        tmp[bucket[index] - 1] = output[i];
        sorted[bucket[index] - 1] = arr[i];
        --bucket[index];
    }
    memcpy(arr, sorted, sizeof(uint64_t) * sz);
    memcpy(output, tmp, sizeof(uint64_t) * sz);
}

void radix_sort(uint64_t* arr, size_t sz, arena_t* arena, uint64_t* output)
{
    assert(arr);
    uint64_t max = array_max_value(arr, sz);

    // Keep looping based upon the number of digits of the max value in the array.
    uint64_t* temp_arr = ARENA_MAKE_ARRAY(arena, uint64_t, sz, 0);
    for (uint64_t i = 0; i < sz; ++i)
    {
        output[i] = i;
    }
    memcpy(temp_arr, arr, sz * sizeof(uint64_t));
    for (int pos = 1; max / pos > 0; pos *= 10)
    {
        count_sort(temp_arr, sz, pos, arena, output);
    }
}