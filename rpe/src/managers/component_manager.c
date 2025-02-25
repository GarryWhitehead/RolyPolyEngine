/* Copyright (c) 2024-2025 Garry Whitehead
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

#include "component_manager.h"

#include "managers/object_manager.h"

#include <utility/hash.h>

rpe_component_manager_t* rpe_comp_manager_init(arena_t* arena)
{
    rpe_component_manager_t* m = ARENA_MAKE_ZERO_STRUCT(arena, rpe_component_manager_t);
    m->objects = HASH_SET_CREATE(uint64_t, uint64_t, arena);
    MAKE_DYN_ARRAY(uint64_t, arena, RPE_COMPONENT_MANAGER_MAX_FREE_ID_COUNT, &m->free_slots);
    m->index = 0;
    return m;
}

uint64_t rpe_comp_manager_add_obj(rpe_component_manager_t* m, rpe_object_t obj)
{
    assert(m);
    uint64_t ret_idx = 0;

    // If there are free slots and the threshold has been reached,
    // use an empty slot.
    if (m->free_slots.size && m->free_slots.size > RPE_OBJ_MANAGER_MIN_FREE_IDS)
    {
        ret_idx = DYN_ARRAY_POP_BACK(uint64_t, &m->free_slots);
        HASH_SET_INSERT(&m->objects, &obj.id, &ret_idx);
    }
    else
    {
        HASH_SET_INSERT(&m->objects, &obj.id, &m->index);
        ret_idx = m->index++;
    }
    return ret_idx;
}

uint64_t rpe_comp_manager_get_obj_idx(rpe_component_manager_t* m, rpe_object_t obj)
{
    uint64_t* obj_idx = HASH_SET_GET(&m->objects, &obj.id);
    if (!obj_idx)
    {
        return UINT64_MAX;
    }
    return *obj_idx;
}

bool rpe_comp_manager_has_obj(rpe_component_manager_t* m, rpe_object_t obj)
{
    if (HASH_SET_FIND(&m->objects, &obj.id))
    {
        return true;
    }
    return false;
}

bool rpe_comp_manager_remove(rpe_component_manager_t* m, rpe_object_t obj)
{
    uint64_t* obj_idx = HASH_SET_GET(&m->objects, &obj.id);
    if (!obj_idx)
    {
        return false;
    }
    DYN_ARRAY_APPEND(&m->free_slots, &obj_idx);
    HASH_SET_ERASE(&m->objects, &obj.id);

    return true;
}