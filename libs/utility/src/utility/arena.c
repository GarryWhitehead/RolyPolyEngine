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

#include "arena.h"

#include <assert.h>
#include <log.h>
#include <string.h>

/* Arena allocator functions */

int arena_new(uint64_t capacity, arena_t* new_arena)
{
#if ARENA_ALLOCATOR_MEM_TYPE == ARENA_MEM_TYPE_STDLIB
    new_arena->begin = malloc(capacity);
    if (!new_arena->begin)
    {
        return ARENA_ERROR_ALLOC_FAILED;
    }
#elif ARENA_ALLOCATOR_MEM_TYPE == ARENA_MEM_TYPE_VMEM
#if !defined(WIN32)
    new_arena->begin =
        mmap(NULL, capacity, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (new_arena->begin == MAP_FAILED)
    {
        return ARENA_ERROR_ALLOC_FAILED;
    }
#else
    new_arena->begin = VirtualAllocEx(
        GetCurrentProcess(), NULL, capacity, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (INV_HANDLE(new_arena->begin))
    {
        return ARENA_ERROR_ALLOC_FAILED;
    }
#endif
#endif

    new_arena->end = new_arena->begin ? new_arena->begin + capacity : 0;
    new_arena->offset = 0;
    return ARENA_SUCCESS;
}

void* arena_alloc(arena_t* arena, ptrdiff_t type_size, ptrdiff_t align, ptrdiff_t count, int flags)
{
    assert(arena->begin && arena->end);

    ptrdiff_t padding = -(uintptr_t)arena->begin & (align - 1);
    uint8_t* offset_ptr = arena->begin + arena->offset;
    ptrdiff_t available = arena->end - offset_ptr - padding;
    if (available < 0 || count > available / type_size)
    {
        log_error(
            "Arena out of memory - available = %i; Required allocation size: %i",
            available,
            count * type_size);
        if (flags & ARENA_OUT_OF_MEM_SOFT_FAIL)
        {
            return NULL;
        }
        abort();
    }
    uint8_t* padded_ptr = offset_ptr + padding;
    arena->offset += padding + count * type_size;
#if ENABLE_DEBUG_ARENA
    log_info(
        "[Arena Allocation Log] Alloc Size: %lu; Current Size: %lu; Available: %lu",
        count * type_size,
        arena->offset,
        available);
#endif
    return flags & ARENA_ZERO_MEMORY ? memset(padded_ptr, 0, count * type_size) : padded_ptr;
}

uint64_t arena_current_size(arena_t* arena) { return (uint64_t)arena->offset; }

void arena_reset(arena_t* arena)
{
    assert(arena->begin && arena->end);
    arena->offset = 0;
}

void arena_release(arena_t* arena)
{
#if ARENA_ALLOCATOR_MEM_TYPE == ARENA_MEM_TYPE_STDLIB
    free(arena->begin);
#elif ARENA_ALLOCATOR_MEM_TYPE == ARENA_MEM_TYPE_VMEM
#if !defined(WIN32)
    int ret = munmap(arena->begin, arena->end - arena->begin);
    assert(ret == 0);
#else
    if (INV_HANDLE(arena->begin))
    {
        return;
    }

    BOOL free_result = VirtualFreeEx(GetCurrentProcess(), (LPVOID)arena->begin, 0, MEM_RELEASE);

    if (FALSE == free_result)
    {
        assert(0 && "VirtualFreeEx() failed.");
    }
}
#endif
#endif

    arena->begin = NULL;
    arena->end = NULL;
    arena->offset = 0;
}

/* Dynamic array allocator functions */

void* _offset_ptr(void* ptr, size_t offset, size_t type_size) { return ptr + offset * type_size; }

int dyn_array_init(
    arena_t* arena,
    uint32_t capcity,
    uint32_t type_size,
    size_t align,
    arena_dyn_array_t* new_dyn_array)
{
    assert(arena);
    assert(type_size > 0);
    assert(align > 0);

    new_dyn_array->capacity = capcity;
    new_dyn_array->size = 0;
    new_dyn_array->arena = arena;
    new_dyn_array->type_size = type_size;
    new_dyn_array->align_size = align;
    new_dyn_array->data = arena_alloc(arena, type_size, align, capcity, ARENA_NONZERO_MEMORY);
    if (!new_dyn_array->data)
    {
        return ARENA_ERROR_ALLOC_FAILED;
    }
    return ARENA_SUCCESS;
}

void* dyn_array_append(arena_dyn_array_t* arr, void* item)
{
    if (arr->size >= arr->capacity)
    {
        dyn_array_grow(arr);
    }
    void* ptr = _offset_ptr(arr->data, arr->size, arr->type_size);
    ++arr->size;
    assert((uintptr_t)arr->data < (uintptr_t)arr->arena->end);
    memcpy(ptr, item, arr->type_size);
    return ptr;
}

void dyn_array_grow(arena_dyn_array_t* arr)
{
    arr->capacity = arr->capacity ? arr->capacity : 1;

    arr->capacity *= 2;
    void* data = arena_alloc(
        arr->arena, arr->type_size, arr->align_size, arr->capacity, ARENA_NONZERO_MEMORY);

    if (arr->size)
    {
        memcpy(data, arr->data, arr->type_size * arr->size);
    }
    arr->data = data;
}

void dyn_array_remove(arena_dyn_array_t* arr, uint32_t idx)
{
    assert(idx < arr->size);
    assert(arr->size > 0);
    assert(arr->data);

    // Easy if the index is the back of the array, just decrease the size.
    if (idx == arr->size - 1)
    {
        --arr->size;
        return;
    }

    void* left_ptr = _offset_ptr(arr->data, idx, arr->type_size);
    void* right_ptr = left_ptr + arr->type_size;
    uint32_t copy_size = arr->size - (idx + 1);
    memcpy(left_ptr, right_ptr, copy_size * arr->type_size);
    --arr->size;
}

void dyn_array_swap(arena_dyn_array_t* arr_dst, arena_dyn_array_t* arr_src)
{
    assert(arr_src->data);
    assert(arr_dst->data);
    assert(arr_src->type_size == arr_dst->type_size);
    assert(arr_src->align_size == arr_dst->align_size);

    if (!arr_src->size && !arr_dst->size)
    {
        return;
    }

    arena_dyn_array_t* small;
    arena_dyn_array_t* big;

    if (arr_src->size > arr_dst->size)
    {
        big = arr_src;
        small = arr_dst;
    }
    else
    {
        big = arr_dst;
        small = arr_src;
    }

    if (big->size >= small->capacity)
    {
        dyn_array_grow(small);
    }

    void* tmp = arena_alloc(small->arena, small->type_size, small->align_size, 1, 0);
    for (uint32_t i = 0; i < small->size; ++i)
    {
        void* small_ptr = _offset_ptr(small->data, i, small->type_size);
        void* big_ptr = _offset_ptr(big->data, i, big->type_size);
        memcpy(tmp, big_ptr, big->type_size);
        memcpy(big_ptr, small_ptr, small->type_size);
        memcpy(small_ptr, tmp, small->type_size);
    }
    uint32_t remaining = big->size - small->size;
    for (uint32_t i = 0; i < remaining; ++i)
    {
        void* small_ptr = _offset_ptr(small->data, i, small->type_size);
        void* big_ptr = _offset_ptr(big->data, i, big->type_size);
        memcpy(small_ptr, big_ptr, small->type_size);
    }
}

void* dyn_array_get(arena_dyn_array_t* arr, uint32_t idx)
{
    assert(idx < arr->size);
    return _offset_ptr(arr->data, idx, arr->type_size);
}

void* dyn_array_pop_back(arena_dyn_array_t* arr)
{
    void* out = NULL;
    if (arr->size > 0)
    {
        out = dyn_array_get(arr, arr->size - 1);
        --arr->size;
    }
    return out;
}

void dyn_array_set(arena_dyn_array_t* arr, uint32_t idx, void* item)
{
    assert(idx < arr->size);
    void* ptr = _offset_ptr(arr->data, idx, arr->type_size);
    memcpy(ptr, item, arr->type_size);
}

void dyn_array_clear(arena_dyn_array_t* dyn_array) { dyn_array->size = 0; }
