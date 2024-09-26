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

#ifndef __UTIL_FILESYSTEM_H__
#define __UTIL_FILESYSTEM_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// Forward declarations.
typedef struct Arena arena_t;
typedef struct String string_t;

typedef struct FsBuffer
{
    char* buffer;
    size_t size;
} fs_buffer_t;

size_t fs_get_file_size(FILE* fp);

fs_buffer_t fs_load_file_into_memory(const char* path, arena_t* arena);

void fs_destroy_buffer(fs_buffer_t* b);

bool fs_get_extension(string_t* path, string_t* out_ext, arena_t*arena);

#endif