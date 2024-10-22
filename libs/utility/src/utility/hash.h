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

#ifndef __UTILITY_HASH_H__
#define __UTILITY_HASH_H__

#include <stdint.h>
#include <string.h>

static uint32_t rotl32(uint32_t x, int8_t r) { return (x << r) | (x >> (32 - r)); }

#define ROTL32(x, y) rotl32(x, y)

#define BIG_CONSTANT(x) (x##LLU)

/**
 A 32bit mumur3 hasher adapted from here:
 https://github.com/PeterScott/murmur3 This is for the use with smaller keys
 with an improvement in performance compared to 128bit versions
 */
static uint32_t murmur_hash3(uint32_t* key, uint32_t size)
{
    assert(key);
    assert(size > 0);

    size_t nblocks = size / 4;

    uint32_t h1 = 0;
    uint32_t c1 = 0xcc9e2d51;
    uint32_t c2 = 0x1b873593;
    uint32_t c3 = 0x85ebca6b;
    uint32_t c4 = 0xc2b2ae35;

    // body
    do
    {
        uint32_t k1;
        memcpy((void*)&k1, (void*)key, sizeof(uint32_t));
        key++;
        k1 *= c1;
        k1 = ROTL32(k1, 15);
        k1 *= c2;
        h1 ^= k1;
        h1 = ROTL32(h1, 13);
        h1 = (h1 * 5) + 0xe6546b64;
    } while (--nblocks);

    // tail
    h1 ^= nblocks;
    h1 ^= h1 >> 16;
    h1 *= c3;
    h1 ^= h1 >> 13;
    h1 *= c4;
    h1 ^= h1 >> 16;

    return h1;
}


#endif
