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

#ifndef __UTILITY_ARENA_H__
#define __UTILITY_ARENA_H__

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define ARENA_MEM_TYPE_STDLIB 0
#define ARENA_MEM_TYPE_VMEM 1

#ifndef ARENA_ALLOCATOR_MEM_TYPE
#define ARENA_ALLOCATOR_MEM_TYPE ARENA_MEM_TYPE_VMEM
#endif

#if ARENA_ALLOCATOR_MEM_TYPE == ARENA_MEM_TYPE_VMEM
#if !defined(WIN32)
#define __USE_MISC
#include <sys/mman.h>
#include <unistd.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define INV_HANDLE(x) (((x) == NULL) || ((x) == INVALID_HANDLE_VALUE))
#endif
#endif

/**
 Flags which specify the behaviour of the arena.
 */
enum
{
    ARENA_ZERO_MEMORY = 1 << 0,
    ARENA_NONZERO_MEMORY = 1 << 1,
    ARENA_OUT_OF_MEM_SOFT_FAIL = 1 << 2,
    ARENA_OUT_OF_MEM_HARD_FAIL = 1 << 3
};

enum
{
    ARENA_SUCCESS,
    ARENA_ERROR_ALLOC_FAILED
};

typedef struct
{
    uint8_t* begin;
    uint8_t* end;
    ptrdiff_t offset;
} arena_t;

/**
 Create a new space allocation in an arena.
 @param arena A pointer to an initialised arena object.
 @param type The type that the space will hold.
 @param size The number of elements to allocate (bytes = sizeof(type) * size)
 @param flags See flags above.
 */
#define ARENA_MAKE_ARRAY(arena, type, size, flags)                                                 \
    (type*)arena_alloc(arena, sizeof(type), _Alignof(type), size, flags)

/**
 Create a new space (zeroed) allocation in an arena.
 @param arena A pointer to an initialised arena object.
 @param type The type that the space will hold.
 @param size The number of elements to allocate (bytes = sizeof(type) * size)
 */
#define ARENA_MAKE_ZERO_ARRAY(arena, type, size)                                                   \
    (type*)arena_alloc(arena, sizeof(type), _Alignof(type), size, ARENA_ZERO_MEMORY)

/**
 Create a single allocation in an arena. Useful when wishing to allocate space for structs.
 @param arena A pointer to an initialised arena object.
 @param type The type that the space will hold.
 @param flags See flags above.
 */
#define ARENA_MAKE_STRUCT(arena, type, size, flags)                                                \
    (type*)arena_alloc(arena, sizeof(type), _Alignof(type), 1, flags)

/**
 Create a single (zeroed) allocation in an arena. Useful when wishing to allocate space for structs.
 @param arena A pointer to an initialised arena object.
 @param type The type that the space will hold.
 */
#define ARENA_MAKE_ZERO_STRUCT(arena, type)                                                        \
    (type*)arena_alloc(arena, sizeof(type), _Alignof(type), 1, ARENA_ZERO_MEMORY)

/**
 Create a new arena allocator instance.
 @param capacity The size of the arena in bytes.
 @param [in,out] new_arena The arena object to initialise.
 @returns an error code. If not ARENA_SUCCESS, initialisation was unsuccessful.
 */
int arena_new(ptrdiff_t capacity, arena_t* new_arena);

/**
 Allocate a new space within the arena. If the required allocation exceeds the reserved arena memory
 space, then the behaviour of the arena depends on the flag passed. Default behaviour is to hard
 fail.
 @param arena A pointer to an initialised arena object.
 @param type_size The size of the type this space will hold in bytes.
 @param align The alignment of the type.
 @param count The number of elements this allocation will be reserved (in bytes = count *
 type_size).
 @param flags See the flags above.
 @return A pointer to the allocated memory space within the arena.
 */
void* arena_alloc(arena_t* arena, ptrdiff_t type_size, ptrdiff_t align, ptrdiff_t count, int flags);

/**
 Get the current used space of the arena.
 @param arena A pointer to a initialised arena.
 @returns the used space in bytes.
 */
uint64_t arena_current_size(arena_t* arena);

/**
 Reset the used memory space of the arena to zero.
 @note No memory de-allocations are performed.
 @param arena A pointer to an arena object.
 */
void arena_reset(arena_t* arena);

/**
 De-allocate the memory used by an arena.
 @param arena A pointer to an arena object.
 */
void arena_release(arena_t* arena);

/* ====================== Dynamic array allocator ========================== */

#define MAKE_DYN_ARRAY(type, arena, size, new_dyn_array)                                           \
    dyn_array_init(arena, sizeof(type), _Alignof(type), size, new_dyn_array)
#define DYN_ARRAY_APPEND(type, arena, dyn_array, item)                                             \
    dyn_array_append(arena, dyn_array, sizeof(type), _Alignof(type), (type*)item)
#define DYN_ARRAY_GET(type, dyn_array, idx) *(type*)dyn_array_get(dyn_array, idx, sizeof(type))

/* Convenience macros for appending items of different types to a dynamic arena array */
#define DYN_ARRAY_APPEND_INT(arena, dyn_array, item)                                               \
    dyn_array_append(arena, dyn_array, sizeof(int), _Alignof(int), (int*)item)
#define DYN_ARRAY_APPEND_UINT(arena, dyn_array, item)                                              \
    dyn_array_append(arena, dyn_array, sizeof(uint32_t), _Alignof(uint32_t), (uint32_t*)item)
#define DYN_ARRAY_APPEND_FLOAT(arena, dyn_array, item)                                             \
    dyn_array_append(arena, dyn_array, sizeof(float), _Alignof(float), (float*)item)

#define DYN_ARRAY_APPEND_CHAR(arena, dyn_array, item)                                              \
    {                                                                                              \
        const char* str = (const char*)item;                                                       \
        dyn_array_append(arena, dyn_array, sizeof(char*), _Alignof(char*), &(str));                \
    }

typedef struct DynArray
{
    uint32_t size;
    uint32_t capacity;
    void* data;
} arena_dyn_array_t;

/**
 Create a dynamic array using a arena allocator.
 @param arena The arena which  the dynamic array will obtain the memory space from.
 @param type_size The size of the type the dynamic array will hold.
 @param align The alignment size of the type.
 @param capcity The number of elements to initially allocate space for. If the capacity is exceeded,
 a new arena allocation occurs.
 @param new_dyn_array A pointer to the dynamic array object to initialise.
 @returns an error code.
 */
int dyn_array_init(
    arena_t* arena,
    ptrdiff_t type_size,
    ptrdiff_t align,
    uint32_t capcity,
    arena_dyn_array_t* new_dyn_array);

/**
 Checks whether a dynamic array needs a re-allocation.
 @param dyn_array A pointer to a dynamic array object.
 @param type_size The size of the type the dynamic array holds.
 @param align The alignment size of the type.
 @param arena A pointer to an arena object.
 */
void dyn_array_grow(
    arena_dyn_array_t* dyn_array, ptrdiff_t type_size, ptrdiff_t align, arena_t* arena);

/**
 Append an item to the dynamic array.
 @param arena A pointer to an arena allocator object.
 @param dyn_array A pointer to the dynamic array which the item will be pushed to.
 @param type_size The size of the item type in bytes.
 @param align The alignment of the item type.
 @param item A void pointer to the item to push onto the array.
 */
void dyn_array_append(
    arena_t* arena, arena_dyn_array_t* dyn_array, ptrdiff_t type_size, ptrdiff_t align, void* item);

/**
 Get an item from a dynamic array.
 @param dyn_array A pointer to the dynamic array which the item will be retrieved from.
 @param idx The index of the item to retrieve.
 @param type_size The size of the item in bytes.
 @returns a void pointer to the retrieved item.
 */
void* dyn_array_get(arena_dyn_array_t* dyn_array, uint32_t idx, ptrdiff_t type_size);

/* ====================== Pool allocator ========================== */

#endif
