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

#ifndef __UTILITY_THREAD_H__
#define __UTILITY_THREAD_H__

#include <stdbool.h>

typedef struct Thread thread_t;
typedef struct Arena arena_t;
typedef void* (*thrd_func)(void*);

thread_t* thread_create(thrd_func func, void* data, arena_t* arena);

bool thread_join(thread_t* t);

thread_t thread_current();

#if WIN32
	#include <threads.h>
	typedef mtx_t mutex_t;
	typedef cnd_t cond_wait_t;
#elif __linux__

#endif

void mutex_init(mutex_t* m);

bool mutex_lock(mutex_t* m);

void mutex_unlock(mutex_t* m);

// The Microsoft docs state that no destruction of a mutex is actually required. So this function does nothing.
inline void mutex_destroy(mutex_t* m) {};

void condition_init(cond_wait_t* c);

bool condition_wait(cond_wait_t* c, mutex_t* m);

bool condition_signal(cond_wait_t* c);

bool condition_brdcast(cond_wait_t* c);

inline void condition_destroy(cond_wait_t* c) {}

#endif
