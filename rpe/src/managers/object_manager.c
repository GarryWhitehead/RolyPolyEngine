/* Copyright (c) 2022 Garry Whitehead
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

#include "object_manager.h"

#include "rpe/object.h"

#include <string.h>
#include <utility/arena.h>

rpe_obj_manager_t rpe_obj_manager_init(arena_t* arena)
{
    rpe_obj_manager_t o;
    MAKE_DYN_ARRAY(uint64_t, arena, 50, &o.free_ids);
    o.current_idx = 1;
    o.generations = ARENA_MAKE_ZERO_ARRAY(arena, uint8_t, RPE_OBJ_MANAGER_INDEX_COUNT);
    memset(o.generations, 0, RPE_OBJ_MANAGER_INDEX_COUNT);
    return o;
}

rpe_object_t rpe_obj_manager_make_obj(uint8_t generation, uint32_t index)
{
    rpe_object_t o = {.id = ((generation << RPE_OBJ_MANAGER_INDEX_BITS) | index)};
    return o;
}

bool rpe_obj_manager_is_alive(rpe_obj_manager_t* m, rpe_object_t obj)
{
    assert(m);
    return rpe_obj_manager_get_generation(obj) == m->generations[rpe_obj_manager_get_index(obj)];
}

rpe_object_t rpe_obj_manager_create(rpe_obj_manager_t* m)
{
    assert(m);
    uint64_t id = 0;
    if (m->free_ids.size && m->free_ids.size > RPE_OBJ_MANAGER_MIN_FREE_IDS)
    {
        id = DYN_ARRAY_POP_BACK(uint64_t, &m->free_ids);
    }
    else
    {
        id = m->current_idx++;
    }

    return rpe_obj_manager_make_obj(m->generations[id], id);
}

void rpe_obj_manager_destroy_obj(rpe_obj_manager_t* m, rpe_object_t obj)
{
    assert(m);
    uint32_t index = rpe_obj_manager_get_index(obj);
    DYN_ARRAY_APPEND(&m->free_ids, &index);
    m->generations[index]++;
}

uint32_t rpe_obj_manager_get_index(rpe_object_t obj) { return obj.id & RPE_OBJ_MANAGER_INDEX_MASK; }

uint8_t rpe_obj_manager_get_generation(rpe_object_t obj)
{
    return (obj.id >> RPE_OBJ_MANAGER_INDEX_BITS) & RPE_OBJ_MANAGER_GENERATION_MASK;
}
