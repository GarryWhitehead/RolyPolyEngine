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

#include "backboard.h"

#include <assert.h>
#include <utility/hash.h>

rg_backboard_t rg_backboard_init(arena_t* arena)
{
    rg_backboard_t i;
    i.backboard = HASH_SET_CREATE(const char*, rg_handle_t, arena, murmur_hash3_string);
    i.arena = arena;
    return i;
}

void rg_backboard_add(rg_backboard_t* bb, const char* name, rg_handle_t handle)
{
    assert(bb);
    hash_set_insert(&bb->backboard, (void*)name, &handle);
}

rg_handle_t rg_backboard_get(rg_backboard_t* bb, const char* name)
{
    assert(bb);
    rg_handle_t* h = hash_set_get(&bb->backboard, (void*)name);
    assert(h);
    return *h;
}

void rg_backboard_remove(rg_backboard_t* bb, const char* name)
{
    assert(bb);
    rg_handle_t* h = hash_set_erase(&bb->backboard, (void*)name);
    assert(h);
}

void rg_backboard_reset(rg_backboard_t* bb)
{
    assert(bb);
    hash_set_clear(&bb->backboard);
}
