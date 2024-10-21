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

#ifndef __RPE_RG_BACKBOARD_H__
#define __RPE_RG_BACKBOARD_H__

#include "render_graph_handle.h"

#include <utility/hash_set.h>
#include <utility/string.h>

typedef struct BackBoard
{
    hash_set_t backboard;
    arena_t* arena;

} rg_backboard_t;

rg_backboard_t rg_backboard_init(arena_t* arena);

void rg_backboard_add(rg_backboard_t* bb, const char* name, rg_handle_t handle);

rg_handle_t rg_backboard_get(rg_backboard_t* bb, string_t name);

void rg_backboard_remove(rg_backboard_t* bb, string_t name);

void rg_backboard_reset(rg_backboard_t* bb);

#endif
