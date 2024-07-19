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

#ifndef __UTILITY_RANDOM_H__
#define __UTILITY_RANDOM_H__

#include <stdint.h>

/**
 Xoroshiro128+ generator - adapted from: https://prng.di.unimi.it/xoroshiro128plus.c
 */
typedef struct XoroRand
{
    uint64_t state[2];
} xoro_rand_t;

static xoro_rand_t xoro_rand_init(uint64_t s0, uint64_t s1)
{
    xoro_rand_t r;
    r.state[0] = s0;
    r.state[1] = s1;
    return r;
}

static uint64_t xoro_rand_rotl(uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }

static void xoro_rand_incr(xoro_rand_t* r)
{
    uint64_t s0 = r->state[0];
    uint64_t s1 = r->state[1] ^ s0;
    r->state[0] = xoro_rand_rotl(s0, 55) ^ s1 ^ (s1 << 14);
    r->state[1] = xoro_rand_rotl(s1, 36);
}

static uint64_t xoro_rand_next(xoro_rand_t* r)
{
    uint64_t res = r->state[0] + r->state[1];
    xoro_rand_incr(r);
    return res;
}

#endif
