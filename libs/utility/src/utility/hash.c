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

#include "hash.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>

uint32_t murmur_hash3(void* key, uint32_t size)
{
    assert(key);
    assert(size > 0);

    uint32_t tmp = 0;
    size_t nblocks = size / sizeof(uint32_t);
    uint32_t* key_32 = (uint32_t*)key;

    // For value types <= 3, pad into four bytes to avoid an out-of-bounds memory copy below.
    if (size <= 3)
    {
        memcpy(&tmp, key, size);
        nblocks = 1;
        key_32 = &tmp;
    }

    uint32_t h1 = 0;
    uint32_t c1 = 0xcc9e2d51;
    uint32_t c2 = 0x1b873593;
    uint32_t c3 = 0x85ebca6b;
    uint32_t c4 = 0xc2b2ae35;

    // body
    do
    {
        uint32_t k1;
        memcpy((void*)&k1, (void*)key_32, sizeof(uint32_t));
        key_32++;
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

uint32_t murmur_hash3_string(void* key, uint32_t)
{
    uint32_t str_size = strlen((const char*)key);
    return murmur_hash3(key, str_size);
}