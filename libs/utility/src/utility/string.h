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
} string_t;

string_t string_init(const char* str, arena_t* arena);

string_t string_copy(string_t* other, arena_t* arena);

bool string_cmp(string_t* a, string_t* b);

string_t string_substring(string_t* s, uint32_t start, uint32_t end, arena_t* arena);

char* string_contains(string_t* s, const char* sub);

string_t** string_split(string_t* s, char literal, uint32_t* count, arena_t* arena);

string_t string_append(string_t* a, const char* b, arena_t* arena);

string_t string_append3(string_t* a, const char* b, const char* c, arena_t* arena);

uint32_t string_find_first_of(string_t* str, char c);

uint32_t string_find_last_of(string_t* str, char c);

uint32_t string_count(string_t* str, const char* cs);

string_t string_remove(string_t* str, uint32_t start_idx, uint32_t end_idx, arena_t* arena);

string_t string_trim(string_t* str, char c, arena_t* arena);

string_t string_replace(string_t* str, const char* orig, const char* rep, arena_t* arena);

#endif