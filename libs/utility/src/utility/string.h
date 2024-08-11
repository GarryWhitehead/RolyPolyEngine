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

#ifndef __UTILITY_STRING_H__
#define __UTILITY_STRING_H__

#include <stdbool.h>
#include <stdint.h>

// Forward declarations.
typedef struct Arena arena_t;

typedef struct String
{
    char* data;
    uint32_t len;
    arena_t* arena;
} string_t;

string_t string_init(const char* str, arena_t* arena);

string_t string_copy(string_t* other);

bool string_cmp(string_t* a, string_t* b);

string_t string_substring(string_t* s, uint32_t start, uint32_t end);

char* string_contains(string_t* s, const char* sub);

string_t** string_split(string_t* s, char literal, uint32_t* count);

#endif