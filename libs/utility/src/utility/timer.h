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

#ifndef __UTILITY_TIMER_H__
#define __UTILITY_TIMER_H__

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#if __linux__
#include <syscall.h>
#include <time.h>
#include <unistd.h>
#elif WIN32
#include <Windows.h>
#endif

typedef struct Timer
{
    struct timespec start;
    struct timespec end;
    bool running;
} util_timer_t;

static inline struct timespec _get_time_ns()
{
#if __linux__
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);

    clockid_t cid = CLOCK_REALTIME;
    clock_gettime(cid, &ts);
    return ts;
#elif WIN32
    LARGE_INTEGER l_counter;
    LARGE_INTEGER l_freq;
    QueryPerformanceCounter(&l_counter);
    QueryPerformanceFrequency(&l_freq);

    // Prevents a 64-bit integer overflow - see:
    // https://github.com/floooh/sokol/blob/189843bf4f86969ca4cc4b6d94e793a37c5128a7/sokol_time.h#L204
    int64_t q = l_counter.QuadPart / l_freq.QuadPart;
    int64_t r = l_counter.QuadPart % l_freq.QuadPart;
    return q * 1000000000 + r * 1000000000 / l_freq.QuadPart;
#endif
}

static inline double sub_timespec_secs(util_timer_t* timer)
{
    return (double)(timer->end.tv_sec - timer->start.tv_sec) +
        (double)(timer->end.tv_nsec - timer->start.tv_nsec) * 1.0e-9;
}

static inline util_timer_t util_timer_init()
{
    util_timer_t timer = {0};
    return timer;
}

static inline void util_timer_start(util_timer_t* timer)
{
    timer->start = _get_time_ns();
    timer->running = true;
}

static inline void util_timer_end(util_timer_t* timer)
{
    assert(timer);
    assert(timer->running);
    timer->end = _get_time_ns();
    timer->running = false;
}

static inline void util_timer_reset(util_timer_t* timer)
{
    assert(timer);
    memset(&timer->start, 0, sizeof(struct timespec));
    memset(&timer->end, 0, sizeof(struct timespec));
    timer->running = false;
}

static inline double util_timer_get_time_secs(util_timer_t* timer)
{
    assert(timer);
    assert(!timer->running);
    return sub_timespec_secs(timer);
}

static inline double util_timer_get_time_ms(util_timer_t* timer)
{
    assert(timer);
    assert(!timer->running);
    return sub_timespec_secs(timer) * 1e6;
}

static inline double util_timer_get_time_ns(util_timer_t* timer)
{
    assert(timer);
    assert(!timer->running);
    return sub_timespec_secs(timer) * 1e9;
}

#endif