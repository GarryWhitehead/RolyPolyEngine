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

int arena_new(ptrdiff_t capacity, arena_t* new_arena)
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

int dyn_array_init(
    arena_t* arena,
    ptrdiff_t type_size,
    ptrdiff_t align,
    uint32_t capcity,
    arena_dyn_array_t* new_dyn_array)
{
    new_dyn_array->capacity = capcity;
    new_dyn_array->size = 0;
    new_dyn_array->data = arena_alloc(arena, type_size, align, capcity, ARENA_NONZERO_MEMORY);
    if (!new_dyn_array->data)
    {
        return ARENA_ERROR_ALLOC_FAILED;
    }
    return ARENA_SUCCESS;
}

void dyn_array_append(
    arena_t* arena, arena_dyn_array_t* dyn_array, ptrdiff_t type_size, ptrdiff_t align, void* item)
{
    if (dyn_array->size >= dyn_array->capacity)
    {
        dyn_array_grow(dyn_array, type_size, align, arena);
    }
    void* ptr = dyn_array->data + dyn_array->size++ * type_size;
    assert((uintptr_t)dyn_array->data < (uintptr_t)arena->end);
    memcpy(ptr, item, type_size);
}

void dyn_array_grow(
    arena_dyn_array_t* dyn_array, ptrdiff_t type_size, ptrdiff_t align, arena_t* arena)
{
    dyn_array->capacity = dyn_array->capacity ? dyn_array->capacity : 1;

    dyn_array->capacity *= 2;
    void* data = arena_alloc(arena, type_size, align, dyn_array->capacity, ARENA_NONZERO_MEMORY);

    if (dyn_array->size)
    {
        memcpy(data, dyn_array->data, type_size * dyn_array->size);
    }

    dyn_array->data = data;
}

void* dyn_array_get(arena_dyn_array_t* dyn_array, uint32_t idx, ptrdiff_t type_size)
{
    assert(idx < dyn_array->size);
    return dyn_array->data + idx * type_size;
}
