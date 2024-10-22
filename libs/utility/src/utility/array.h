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

#ifndef __UTILITY_ARRAY_H__
#define __UTILITY_ARRAY_H__

#include <stdint.h>

typedef struct Array
{
    /// Number of elements this array can hold.
    uint32_t size;
    /// The size of the element type in bytes.
    uint32_t type_size;
    /// A void pointer to the array memory space.
    void* data;
} array_t;

/**
 Initialise a new array instance.
 @param type The type of the elements this array will hold.
 @param size The number of elements this array can hold.
 */
#define ARRAY_INIT(type, size) array_init(size, sizeof(type))

/**
 Assign a item to the array at the specified index.
 @param type The type of the item to assign (and the array was allocated with).
 @param arr A pointer to an initialised array struct.
 @param idx The index into the array to assign the item to.
 @param item The item to assign.
 */
#define ARRAY_ASSIGN(type, arr, idx, item) array_assign(arr, idx, (type*)item)

/**
 Get an item from the array.
 @param type The type of the item to retrieve from the array.
 @param arr A pointer to an initialised array struct.
 @param idx An index into the array to retrieve the item from.
 */
#define ARRAY_GET(type, arr, idx) *(type*)array_get(arr, idx)

/**
 Step through the array from idx 0...n
 An example usage - assign an ascending `num` to each item in an array:

   num = 0;
   ARRAY_FOR_EACH(int, *val, array)
   {
       *val = num++;
   }

 @param type The type of elements this array holds.
 @param item The item retrieved from the array at the current index in the for loop.
 */
#define ARRAY_FOR_EACH(type, item, array)                                                          \
    for (int keep = 1, count = 0, size = (array.size); keep && count != size;                      \
         keep = !keep, count++)                                                                    \
        for (type item = ((uint8_t*)array.data) + count * sizeof(type); keep; keep = !keep)

/**
 Initialise a new array instance.
 @param size The number of elements the array will hold.
 @param type_size The type of the elements in bytes.
 @return A initialised array struct.
 */
array_t array_init(uint32_t size, uint32_t type_size);

/**
 Assign a item at a given index.
 @param array A pointer to an array struct.
 @param idx The index into the array to assign the item to.
 @param item A void pointer to the item to assign.
 */
void array_assign(array_t* array, uint32_t idx, void* item);

/**
 Get an item from the array.
 @param array A pointer to an initialised array struct.
 @param idx The index to get the item from in the array.
 @returns a void pointer to the item.
 */
void* array_get(array_t* array, uint32_t idx);

/**
 Zero the array memory space.
 @param array A pointer to an initialised array struct.
 */
void array_clear(array_t* array);

/**
 Deallocate the resources for a given array.
 @param array A pointer to an initialised array struct.
 */
void array_free(array_t* array);

#endif