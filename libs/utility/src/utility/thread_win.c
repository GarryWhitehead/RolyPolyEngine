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
#include "thread.h"

#include <threads.h>

typedef struct Thread
{
    thrd_t handle;
} thread_t;

thread_t* thread_create(thrd_func func, void* data, arena_t* arena)
{
    thread_t* t = ARENA_MAKE_STRUCT(arena, thread_t, ARENA_ZERO_MEMORY);
    int err = thrd_create(&t->handle, func, data);
    if (err != thrd_success)
    {
        return NULL;
    }
    return t;
}

bool thread_join(thread_t* t)
{
    assert(t);
    int res = thrd_join(t->handle, NULL);
    if (res != thrd_success)
    {
        return false;
    }
    return true;
}

thread_t thread_current()
{
    thread_t t;
    t.handle = thrd_current();
    return t;
}

void mutex_init(mutex_t* m) { memset(m, 0, sizeof(mutex_t)); }

bool mutex_lock(mutex_t* m)
{
    int res = mtx_lock(m);
    if (res != thrd_success)
    {
        return false;
    }
    return true;
}

void mutex_unlock(mutex_t* m) { mtx_unlock(m); }

void condition_init(cond_wait_t* c) { memset(c, 0, sizeof(cond_wait_t)); }

bool condition_wait(cond_wait_t* c, mutex_t* m)
{
    int res = cnd_wait(c, m);
    if (res != thrd_success)
    {
        return false;
    }
    return true;
}

bool condition_signal(cond_wait_t* c)
{
    int res = cnd_signal(c);
    if (res != thrd_success)
    {
        return false;
    }
    return true;
}

bool condition_brdcast(cond_wait_t* c)
{
    int res = cnd_broadcast(c);
    if (res != thrd_success)
    {
        return false;
    }
    return true;
}

void condition_destroy(cond_wait_t* c) {}

void mutex_destroy(mutex_t* m) {}