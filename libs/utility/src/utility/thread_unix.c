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

#include <pthread.h>
#include <string.h>

typedef struct Thread
{
    pthread_t handle;
} thread_t;

thread_t* thread_create(thrd_func func, void* data, arena_t* arena)
{
    thread_t* t = ARENA_MAKE_STRUCT(arena, thread_t, ARENA_ZERO_MEMORY);
    if (pthread_create(&t->handle, NULL, func, data) != 0)
    {
        return NULL;
    }
    return t;
}

bool thread_join(thread_t* t)
{
    assert(t);
    if (pthread_join(t->handle, NULL) != 0)
    {
        return false;
    }
    return true;
}

thread_t thread_current()
{
    thread_t out;
    out.handle = pthread_self();
    return out;
}

void mutex_init(mutex_t* m) { pthread_mutex_init(m, NULL); }

bool mutex_lock(mutex_t* m)
{
    if (pthread_mutex_lock(m) != 0)
    {
        return false;
    }
    return true;
}

void mutex_unlock(mutex_t* m) { pthread_mutex_unlock(m); }

void condition_init(cond_wait_t* c) { pthread_cond_init(c, NULL); }

bool condition_wait(cond_wait_t* c, mutex_t* m)
{
    if (pthread_cond_wait(c, m) != 0)
    {
        return false;
    }
    return true;
}

bool condition_signal(cond_wait_t* c)
{
    if (pthread_cond_signal(c) != 0)
    {
        return false;
    }
    return true;
}

bool condition_brdcast(cond_wait_t* c)
{
    if (pthread_cond_broadcast(c) != 0)
    {
        return false;
    }
    return true;
}

void condition_destroy(cond_wait_t* c) { pthread_cond_destroy(c); }

void mutex_destroy(mutex_t* m) { pthread_mutex_destroy(m); }
