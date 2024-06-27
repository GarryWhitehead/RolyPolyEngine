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

#include "array.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

array_t array_init(uint32_t size, uint32_t type_size)
{
    assert(size > 0);
    assert(type_size > 0);

    array_t array;
    array.size = size;
    array.data = malloc(size * type_size);
    array.type_size = type_size;
    assert(array.data);
    return array;
}

void array_assign(array_t* array, uint32_t idx, void* item)
{
    assert(idx < array->size);
    assert(item);
    assert(array->data);

    void* array_ptr = (uint8_t*)array->data + idx * array->type_size;
    memcpy(array_ptr, item, array->type_size);
}

void* array_get(array_t* array, uint32_t idx)
{
    assert(idx < array->size);
    assert(array->data);

    void* array_ptr = (uint8_t*)array->data + idx * array->type_size;
    return array_ptr;
}

void array_clear(array_t* array)
{
    assert(array);
    assert(array->data);

    memset(array->data, 0, array->size * array->type_size);
}

void array_free(array_t* array)
{
    assert(array->data);
    free(array->data);
    array->data = NULL;
    array->size = 0;
}
