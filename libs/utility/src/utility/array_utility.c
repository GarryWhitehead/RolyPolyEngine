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

#include "array_utility.h"

#include <assert.h>
#include <string.h>

uint8_t* _pointer_offset(uint8_t* data, uint32_t size, uint32_t type_size)
{
    assert(data);
    assert(type_size > 0);

    return data + size * type_size;
}

uint32_t array_util_find(void* val, void* data, uint32_t size, uint32_t type_size)
{
    for (uint32_t i = 0; i < size; ++i)
    {
        uint8_t* ptr = _pointer_offset(data, i, type_size);
        if (memcmp(ptr, val, type_size) == 0)
        {
            return i;
        }
    }
    return UINT32_MAX;
}

uint64_t array_max_value(const uint64_t* arr, size_t sz)
{
    assert(arr);
    assert(sz > 0);
    uint64_t max = arr[0];
    for (size_t i = 1; i < sz; ++i)
    {
        max = arr[i] > max ? arr[i] : max;
    }
    return max;
}