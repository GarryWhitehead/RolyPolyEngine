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
#include <string.h>

uint32_t murmur2_hash(void* data, uint32_t len, uint32_t seed)
{
    assert(len > 0);

    const uint64_t Constant = 0xc6a4a7935bd1e995ULL;
    const int Rotation = 47;

    uint64_t h1 = seed ^ (len * Constant);

    const uint64_t* dataPtr = (uint64_t*)data;
    const uint64_t* endPtr = dataPtr + (len / 8);

    // Process the "body" - 8bytes processed per loop.
    while (dataPtr != endPtr)
    {
        uint64_t k1 = *(dataPtr++);
        k1 *= Constant;
        k1 ^= k1 >> Rotation;
        k1 *= Constant;
        h1 ^= k1;
        h1 *= Constant;
    }

    // Process any remaining bytes - max of 7bytes.
    const uint8_t* tail = (uint8_t*)dataPtr;

    switch (len & 7)
    {
        case 7:
            h1 ^= (uint64_t)tail[6] << 48;
        case 6:
            h1 ^= (uint64_t)tail[5] << 40;
        case 5:
            h1 ^= (uint64_t)tail[4] << 32;
        case 4:
            h1 ^= (uint64_t)tail[3] << 24;
        case 3:
            h1 ^= (uint64_t)tail[2] << 16;
        case 2:
            h1 ^= (uint64_t)tail[1] << 8;
        case 1:
            h1 ^= (uint64_t)tail[0];
            h1 *= Constant;
    }

    // Finalise mix.
    h1 ^= h1 >> Rotation;
    h1 *= Constant;
    h1 ^= h1 >> Rotation;

    return h1;
}

uint32_t murmur2_hash_string(void* key, uint32_t, uint32_t)
{
    uint32_t str_size = strlen((const char*)key);
    return murmur2_hash(key, str_size, 0);
}