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

#include "filesystem.h"
#include "arena.h"
#include "string.h"

#include <log.h>

#include <stdio.h>
#include <assert.h>
#include <malloc.h>

size_t fs_get_file_size(FILE* fp)
{
    assert(fp);
    fseek(fp, 0L, SEEK_END);
    size_t sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    return sz;
}

fs_buffer_t fs_load_file_into_memory(const char* path, arena_t* arena)
{
    fs_buffer_t b;
    b.size = 0;
    b.buffer = NULL;

    FILE* fp = fopen(path, "r");
    if (!fp)
    {
        log_error("Error opening file: %s", path);
        return b;
    }

    b.size = fs_get_file_size(fp);
    if (!b.size)
    {
        log_error("Error loading file into memory: file size is zero.");
        return b;
    }

    b.buffer = arena_alloc(arena, sizeof(char), _Alignof(char), b.size + 1, 0);
    assert(b.buffer);

    for (size_t i = 0; i < b.size; ++i)
    {
        b.buffer[i] = (char)fgetc(fp);
    }
    b.buffer[b.size] = '\0';

    fclose(fp);
    return b;
}

void fs_destroy_buffer(fs_buffer_t* b)
{
    assert(b);
    free(b->buffer);
    b->buffer = NULL;
    b->size = 0;
}

bool fs_get_extension(string_t* path, string_t* out_ext, arena_t*arena)
{
    assert(path);
    uint32_t idx = string_find_last_of(path, '.');
    if (idx == UINT32_MAX)
    {
        return false;
    }
    *out_ext = string_substring(path, idx + 1, path->len - 1, arena);
    return true;
}
