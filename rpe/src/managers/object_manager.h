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

#ifndef __RPE_OBJECT_MANAGER_H__
#define __RPE_OBJECT_MANAGER_H__

#define RPE_OBJ_MANAGER_INDEX_BITS 22
#define RPE_OBJ_MANAGER_INDEX_MASK ((1 << RPE_OBJ_MANAGER_INDEX_BITS) - 1)
#define RPE_OBJ_MANAGER_INDEX_COUNT (1 << RPE_OBJ_MANAGER_INDEX_BITS)

#define RPE_OBJ_MANAGER_GENERATION_BITS 8
#define RPE_OBJ_MANAGER_GENERATION_MASK ((1 << RPE_OBJ_MANAGER_GENERATION_BITS) - 1)

#define RPE_OBJ_MANAGER_MIN_FREE_IDS 1024
#define RPE_OBJ_MANAGER_MAX_OBJECTS 262144

#include "rpe/object.h"

#include <utility/arena.h>
#include <stdint.h>
#include <stdbool.h>

/**
 Based heavily on the implementation described in the bitsquid engine
 blogpost: http://bitsquid.blogspot.com/2014/08/building-data-oriented-entity-system.html
 */
typedef struct ObjectManager
{
    int current_idx;
    arena_dyn_array_t free_ids;
    uint8_t* generations;
} rpe_obj_manager_t;

rpe_obj_manager_t rpe_obj_manager_init(arena_t* arena);

bool rpe_obj_manager_is_alive(rpe_obj_manager_t* m, rpe_object_t obj);

rpe_object_t rpe_obj_manager_create(rpe_obj_manager_t* m);

void rpe_obj_manager_destroy_obj(rpe_obj_manager_t* m, rpe_object_t obj);

uint32_t rpe_obj_manager_get_index(rpe_object_t obj);

uint8_t rpe_obj_manager_get_generation(rpe_object_t obj);

#endif