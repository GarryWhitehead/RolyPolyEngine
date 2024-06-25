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

#include "vector.h"

#include "assert.h"
#include "stdlib.h"
#include "string.h"

void* _vec_ptr_offset(vector_t* vec, uint32_t offset)
{
    assert(vec->data);
    return (uint8_t*)vec->data + offset * vec->type_size;
}

void _vec_reallocate(vector_t* vec, uint32_t new_capacity)
{
    assert(vec->data);

    void* old_data = vec->data;
    // If we have enough allocated memory, it's a no-op.
    if (new_capacity <= vec->size)
    {
        return;
    }

    vec->data = malloc(new_capacity * vec->type_size);
    memcpy(vec->data, old_data, vec->size * vec->type_size);
    free(old_data);

    vec->capacity = new_capacity;
}

void _vec_assign(vector_t* vec, uint32_t idx, void* item)
{
    assert(idx < vec->capacity);
    assert(vec->data);

    void* ptr = _vec_ptr_offset(vec, idx);
    memcpy(ptr, item, vec->type_size);
}

vector_t vector_init(uint32_t capacity, uint32_t type_size)
{
    assert(type_size > 0);
    assert(capacity > 0);

    vector_t vec = {0};
    vec.data = malloc(capacity * type_size);
    vec.capacity = capacity;
    vec.size = 0;
    vec.type_size = type_size;
    return vec;
}

void vector_push_back(vector_t* vec, void* item)
{
    assert(item);

    if (vec->capacity <= vec->size)
    {
        _vec_reallocate(vec, vec->capacity * VEC_GROWTH_FACTOR);
    }
    _vec_assign(vec, vec->size, item);
    ++vec->size;
}

void* vector_pop_back(vector_t* vec)
{
    assert(vec->size >= 1);

    return _vec_ptr_offset(vec, --vec->size);
}

void vector_assign(vector_t* vec, uint32_t idx, void* item)
{
    assert(idx < vec->size);
    assert(item);

    void* ptr = _vec_ptr_offset(vec, idx);
    memcpy(ptr, item, vec->type_size);
}

void* vector_get(vector_t* vec, uint32_t idx)
{
    assert(idx < vec->size);
    return _vec_ptr_offset(vec, idx);
}

void vector_resize(vector_t* vec, uint32_t new_size)
{
    if (new_size > vec->capacity)
    {
        _vec_reallocate(vec, new_size * VEC_GROWTH_FACTOR);
    }
    vec->size = new_size;
}

void vector_clear(vector_t* vec) { vector_resize(vec, 0); }

bool vector_is_empty(vector_t* vec) { return !vec->size; }

void vector_free(vector_t* vec)
{
    assert(vec->data);
    free(vec->data);
    vec->data = NULL;
    vec->size = 0;
    vec->capacity = 0;
}
