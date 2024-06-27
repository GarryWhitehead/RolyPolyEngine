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

#ifndef __UTILITY_VECTOR_H__
#define __UTILITY_VECTOR_H__

#include <stdbool.h>
#include <stdint.h>

#define VEC_GROWTH_FACTOR 2

typedef struct Vector
{
    /// The current size of the vector (in elements).
    uint32_t size;
    /// The current total memory allocation for this vector (in elements).
    uint32_t capacity;
    /// The size of the element type in bytes.
    uint32_t type_size;
    /// A pointer to the memory allocation for this vector.
    void* data;
} vector_t;

/**
 Initialise a new vector instance.
 @param type The type of element this vector will hold.
 @param size The initial capacity of the vector (in elements).
 */
#define VECTOR_INIT(type, size) vector_init(size, sizeof(type))

/**
 Push an item onto the back of the vector. If there is not enough capacity for this operation, a
 memory reallocation occurs.
 @param type The type of element held by the vector.
 @param vec A pointer to an initialised vector struct.
 @param item A pointer to the item to push onto the vector.
 */
#define VECTOR_PUSH_BACK(type, vec, item) vector_push_back(vec, (type*)item)

/**
 Pop an item off the back of the vector. The vector size must be >= 1.
 @param type The type of element held by the vector.
 @param vec A pointer to an initialised vector struct.
 @returns The item 'popped' off the back.
 */
#define VECTOR_POP_BACK(type, vec) *(type*)vector_pop_back(vec)

/**
 Assign an item to the vector at the specified index.
 @param type The type of element held by the vector.
 @param vec A pointer to an initialised vector struct.
 @param idx The index to assign the item to. The index must be less than the vector size.
 @param item The item to assign.
 */
#define VECTOR_ASSIGN(type, vec, idx, item) vector_assign(vec, idx, (type*)item)

/**
 Get an item from the vector.
 @param type The type of element held by the vector.
 @param vec A pointer to an initialised vector struct.
 @param idx The index to get the item from. The index must be less than the vector size.
 */
#define VECTOR_GET(type, vec, idx) *(type*)vector_get(vec, idx)

/**
 Step through the vector from idx 0...n
 An example usage - double the value for each element:

  ARRAY_FOR_EACH(int, *val, vec)
  {
      *val *= 2;
  }

 @param type The type of element held by the vector.
 @param item A pointer to the vector item at the current index in the loop.
 @param vec A pointer to an initialised vector struct.
 */
#define VECTOR_FOR_EACH(type, item, vec)                                                           \
    for (int keep = 1, count = 0, size = (vec.size); keep && count != size; keep = !keep, count++) \
        for (type item = (vec.data) + count * sizeof(type); keep; keep = !keep)

/**
 Initialise a new vector instance.
 @param capacity The initial capacity of the vector (in elements).
 @param type_size The type of element this vector will hold.
 */
vector_t vector_init(uint32_t capacity, uint32_t type_size);

/**
 Push an item onto the back of the vector. If there is not enough capacity for this operation, a
 memory reallocation occurs.
 @param vec A pointer to an initialised vector struct.
 @param item A void pointer to the item to push onto the vector.
 */
void vector_push_back(vector_t* vec, void* item);

/**
 Pop an item off the back of the vector. The vector size must be >= 1.
 @param vec A pointer to an initialised vector struct.
 @returns A void pointer to the item 'popped' off the back.
 */
void* vector_pop_back(vector_t* vec);

/**
 Assign an item to the vector at the specified index.
 @param vec A pointer to an initialised vector struct.
 @param idx The index to assign the item to. The index must be less than the vector size.
 @param item A void pointer to the item to assign.
 */
void vector_assign(vector_t* vec, uint32_t idx, void* item);

/**
 Get an item from the vector.
 @param vec A pointer to an initialised vector struct.
 @param idx The index to get the item from. The index must be less than the vector size.
 */
void* vector_get(vector_t* vec, uint32_t idx);

/**
 Resize the vector. If the new size is greater than the current capacity, a memory re-allocation
 occurs. The data from the old memory space is maintained in the new allocation.
 @param vec A pointer to an initialised vector struct.
 @param new_size The size (in elements) to resize the vector to.
 */
void vector_resize(vector_t* vec, uint32_t new_size);

/**
 Clear the current items in the vector (sets the size to zero, but no memory de-allocations occur).
 @param vec A pointer to an initialised vector struct.
 */
void vector_clear(vector_t* vec);

/**
 Check if the vector currently has no items.
 @param vec A pointer to an initialised vector struct.
 @return If empty, returns true.
 */
bool vector_is_empty(vector_t* vec);

/**
 Deallocate the memory space associated with a vector.
 @param vec A pointer to an initialised vector struct.
 */
void vector_free(vector_t* vec);

#endif