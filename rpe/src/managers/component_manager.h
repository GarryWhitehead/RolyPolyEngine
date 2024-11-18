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

#ifndef __RPE_COMPONENT_MANAGER_H__
#define __RPE_COMPONENT_MANAGER_H__

#include "rpe/object.h"

#include <utility/hash_set.h>
#include <utility/arena.h>

#define RPE_COMPONENT_MANAGER_MAX_FREE_ID_COUNT 1024

#define ADD_OBJECT_TO_MANAGER(arr, handle, obj) \
if (handle >= arr.size) \
    { \
        dyn_array_append(arr, obj); \
    } \
    else \
    { \
        dyn_array_set(arr, handle, obj); \
    }

typedef struct ObjectHandle
{
    uint64_t id;
} rpe_obj_handle_t;

static inline bool rpe_obj_handle_is_valid(rpe_obj_handle_t h)
{
    return h.id != UINT64_MAX;
}

static inline rpe_obj_handle_t rpe_obj_handle_invalid_handle()
{
    rpe_obj_handle_t out = {.id = UINT64_MAX };
    return out;
}

typedef struct ComponentManager
{
    // the Objects which contain this component and their index location
    hash_set_t objects;

    // free buffer indices from destroyed Objects.
    // rather than resize buffers which will be slow, empty slots in manager
    // containers are stored here and re-used
    arena_dyn_array_t free_slots;

    // the current index into the main manager buffers which will be allocated
    // to the next Object that is added.
    uint64_t index;
} rpe_component_manager_t;

rpe_component_manager_t* rpe_comp_manager_init(arena_t* arena);

rpe_obj_handle_t rpe_comp_manager_add_obj(rpe_component_manager_t* m, rpe_object_t obj);

rpe_obj_handle_t rpe_comp_manager_get_obj_idx(rpe_component_manager_t* m, rpe_object_t obj);

bool rpe_comp_manager_has_obj(rpe_component_manager_t* m, rpe_object_t obj);

bool rpe_comp_manager_remove(rpe_component_manager_t* m, rpe_object_t obj);

#endif