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

#ifndef __VKAPI_SHADER_UTIL_H__
#define __VKAPI_SHADER_UTIL_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <utility/string.h>

// Forward declarations.
typedef struct Variant variant_t;

/**
 Laod and append a include file to a shader code block.
 @note glslang offers this functionality via a callback but is long-winded to setup and easier to use our own method.
 @param block The shader code block which will be appended with the include file.
 @param path A path to the include file. Must be a relative path, inside the shader folder.
 @param arena An arena allocator.
 @return If successful, return true.
 */
bool shader_util_append_include_file(string_t* block, string_t* path, arena_t* arena);

/**
 Parse the shader code and process any preprocessor directives based on the specified variants.
 @param block The shader code block.
 @param variants A list of variants.
 @param variant_count The number of variants.
 @param arena An arena allocator (frame lifetime)
 @param scratch_arena An arena allocator (function lifetime).
 @return If there is an error whilst processing, false is returned.
 */
string_t shader_program_process_preprocessor(
    string_t* block,
    variant_t* variants,
    uint32_t variant_count,
    arena_t* arena,
    arena_t* scratch_arena);

/* Private functions */

bool pp_parse_if(
    string_t* line, variant_t* variants, uint32_t variant_count, arena_t* scratch_arena, uint32_t line_idx, bool* error);

string_t read_line(const char* shader_code, uint32_t* idx, uint32_t count, char* buffer);

#endif