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

#include "shader_util.h"

#include "program_manager.h"

#include <assert.h>
#include <log.h>
#include <string.h>
#include <utility/filesystem.h>

#define PP_MAX_TRUE_CONDITIONS 10
#define PP_MAX_FALSE_CONDITIONS 10

typedef struct PreprocessorInfo
{
    string_t true_conds[PP_MAX_TRUE_CONDITIONS];
    uint32_t true_cond_count;
    string_t false_conds[PP_MAX_FALSE_CONDITIONS];
    uint32_t false_cond_count;
} pp_info_t;

string_t read_line(const char* shader_code, uint32_t* idx, uint32_t count, char* buffer)
{
    assert(buffer);
    assert(shader_code);

    int i = 0;
    string_t out;
    out.data = buffer;

    while (*idx < count)
    {
        if (shader_code[*idx] == '\n')
        {
            ++(*idx);
            buffer[i++] = '\0';
            break;
        }
        buffer[i++] = shader_code[(*idx)++];
    }

    buffer[i] = '\0';
    out.len = i;
    return out;
}

bool shader_util_append_include_file(string_t* block, string_t* path, arena_t* arena)
{
    // Check the path is an actual include file.
    string_t ext;
    if (!fs_get_extension(path, &ext, arena))
    {
        log_error("Invalid include file - no extension found.");
        return false;
    }
    if (strcmp(ext.data, "h") != 0)
    {
        log_error("Invalid include file - incorrect file extension: %s", ext.data);
        return false;
    }

    string_t shader_dir = string_init(RPE_SHADER_DIRECTORY, arena);
    string_t abs_path =
        string_append3(&shader_dir, string_init("/", arena).data, path->data, arena);
    fs_buffer_t* fs_buffer = fs_load_file_into_memory(abs_path.data, arena);
    if (!fs_buffer)
    {
        return false;
    }
    *block = string_append(block, fs_get_buffer(fs_buffer), arena);

    return true;
}

bool pp_scan_for_section_end(
    string_t* block,
    uint32_t* idx,
    char* buffer,
    size_t* first_del,
    size_t* second_del,
    size_t* sec_line_sz)
{
    *first_del = UINT64_MAX;
    *second_del = UINT64_MAX;

    while (*idx < block->len)
    {
        string_t line = read_line(block->data, idx, block->len, buffer);
        if (string_contains(&line, "#elif") || string_contains(&line, "#else"))
        {
            *first_del = *first_del == UINT64_MAX ? *idx - line.len : *first_del;
        }
        if (string_contains(&line, "#endif"))
        {
            *second_del = (*idx - line.len) - 1;
            *sec_line_sz = line.len;
            return true;
        }
    }
    log_error("Incorrectly terminated pre-processor statement - no #endif statement found.");
    return false;
}

string_t pp_extract_definition(string_t* line, arena_t* scratch_arena)
{
    assert(line);
    return string_substring(
        line,
        string_find_first_of(line, '(') + 1,
        string_find_first_of(line, ')') - 1,
        scratch_arena);
}

bool pp_contains_variant(string_t* def, variant_t* variants, uint32_t variant_count)
{
    assert(def);
    assert(variants);

    for (uint32_t i = 0; i < variant_count; ++i)
    {
        if (string_cmp(&variants[i].definition, def))
        {
            return true;
        }
    }
    return false;
}

enum
{
    OR_GROUP,
    AND_GROUP,
    OR_DEFINE,
    AND_DEFINE,
    NONE
};
bool pp_compute_if_result(pp_info_t* info, variant_t* variants, uint32_t variant_count, int flag)
{
    // False conditions - if any of the defines are specified in the variant store, then this if
    // statement fails.
    bool res = flag == AND_DEFINE ? true : false;
    for (uint32_t i = 0; i < info->false_cond_count; ++i)
    {
        bool b = pp_contains_variant(&info->false_conds[i], variants, variant_count);
        res = flag == AND_DEFINE ? res | b : flag == OR_DEFINE ? res & b : !b;
    }
    // True conditions - if any of defines are not specified in the variant store, if statement
    // fails.
    for (uint32_t i = 0; i < info->true_cond_count; ++i)
    {
        bool b = pp_contains_variant(&info->true_conds[i], variants, variant_count);
        res = flag == AND_DEFINE ? res & b : flag == OR_DEFINE ? res | b : b;
    }
    return res;
}

bool pp_parse_defines(string_t* line, pp_info_t* info, arena_t* arena, size_t line_idx)
{
    if (string_contains(line, "!defined"))
    {
        info->false_conds[info->false_cond_count++] = pp_extract_definition(line, arena);
    }
    else if (string_contains(line, "defined"))
    {
        info->true_conds[info->true_cond_count++] = pp_extract_definition(line, arena);
    }
    else
    {
        log_error("Invalid pre-processor #elif statement on line %i.", line_idx);
        return false;
    }
    return true;
}

bool pp_parse_if(
    string_t* line,
    variant_t* variants,
    uint32_t variant_count,
    arena_t* scratch_arena,
    uint32_t line_idx,
    bool* error)
{
    *error = false;
    // Tidy the line - remove the #elf part.
    string_t new_line =
        string_substring(line, string_find_first_of(line, ' ') + 1, line->len - 1, scratch_arena);

    // The split function can only deal with single chars so replace the AND/OR operators with
    // single chars.
    new_line = string_replace(&new_line, "&&", "&", scratch_arena);
    new_line = string_replace(&new_line, "||", "|", scratch_arena);

    // Check that we don't have grouped OR statements within an AND, i.e. (defined(THIS) &&
    // defined(THAT)) || defined(SOME)
    uint32_t group_count = 1;
    string_t** group_split;
    int group_flag = NONE;
    int define_flag = NONE;

    // Remove white-space.
    new_line = string_trim(&new_line, ' ', scratch_arena);

    if (string_contains(line, "&") && string_contains(line, "|"))
    {
        uint32_t idx = string_find_first_of(&new_line, '&');
        assert(idx != UINT32_MAX);

        // Check for ))& which states that we have (group) && (group) ...
        if (new_line.data[idx - 1] == ')' && new_line.data[idx - 2] == ')')
        {
            group_split = string_split(&new_line, '&', &group_count, scratch_arena);
            group_flag = AND_GROUP;
            define_flag = OR_DEFINE;
        }
        else
        {
            // Because the && isn't a group but within a group (i.e. (defined(FOO) && defined(BAR))
            // then there must be OR logic between groups. We can assume this for all groups as we
            // only allow either (defined(FOO) && defined(BAR)) || (defined(WHAT)) or  (defined(FOO)
            // || defined(BAR)) && (defined(WHAT))
            group_split = string_split(&new_line, '|', &group_count, scratch_arena);
            group_flag = OR_GROUP;
            define_flag = AND_DEFINE;
        }

        for (uint32_t i = 0; i < group_count; ++i)
        {
            // Check for correctly defined group - i.e. enclosed in brackets: (defined(FOO) ...)
            string_t* str = group_split[i];
            if (str->len && str->data[0] == '(' && str->data[str->len - 1] == ')')
            {
                // Adjust so we don't use the group brackets as this will mess things up later
                // downstream.
                str->data += 1;
                str->len -= 1;
            }
            else
            {
                log_error(
                    "Invalid definition grouping at line: %s. Groups must be enclosed in brackets: "
                    "%s",
                    line_idx,
                    str->data);
                *error = true;
                return false;
            }
        }
    }
    else
    {
        // No separate groups, so just treat as a single.
        group_split = ARENA_MAKE_STRUCT(scratch_arena, string_t*, 0);
        group_split[0] = &new_line;
        define_flag = string_contains(&new_line, "&") ? AND_DEFINE
            : string_contains(&new_line, "|")         ? OR_DEFINE
                                                      : NONE;
    }

    bool final_res = group_flag == OR_GROUP ? false : true;
    bool def_res;

    for (uint32_t i = 0; i < group_count; ++i)
    {
        // First check if multiple pre-processor if statements - i.e. #if defined(THIS) &&
        // defined(THAT)
        uint32_t def_count;
        string_t** def_split = string_split(
            group_split[i], define_flag == AND_DEFINE ? '&' : '|', &def_count, scratch_arena);

        pp_info_t info;
        memset(&info, 0, sizeof(pp_info_t));

        for (uint32_t j = 0; j < def_count; ++j)
        {
            if (!pp_parse_defines(def_split[j], &info, scratch_arena, line_idx))
            {
                *error = true;
                return false;
            }
        }
        // Compute grouped statements separately.
        def_res = pp_compute_if_result(&info, variants, variant_count, define_flag);
        final_res = group_flag == AND_GROUP ? final_res & def_res : final_res | def_res;
    }

    return group_count > 1 ? final_res : def_res;
}

bool pp_edit_shader_block(
    string_t* block,
    uint32_t* idx,
    char* buffer,
    size_t begin_if_idx,
    arena_t* scratch_arena,
    string_t* new_block)
{
    uint32_t end_if_idx = *idx;
    size_t first, second, sec_line_sz;
    if (!pp_scan_for_section_end(block, idx, buffer, &first, &second, &sec_line_sz))
    {
        return false;
    }

    // If 'first' idx is valid, then deleting between other #else/#elif blocks. Will remove #endif
    // statement with remove call as well. Otherwise, just need to remove the #endif statement at
    // the end of the block.
    assert(second != UINT64_MAX);
    size_t a = first != UINT64_MAX ? first : second + 1;
    size_t b = second + sec_line_sz;
    string_t tmp = string_remove(block, a, b, scratch_arena);

    // Remove #if statement from the beginning of text.
    assert(begin_if_idx <= end_if_idx);
    *new_block = string_remove(&tmp, begin_if_idx, end_if_idx - 1, scratch_arena);
    *idx = *idx - (end_if_idx - begin_if_idx) - (b - a) - 1;
    return true;
}

bool pp_parse_preprocessor_branch(
    string_t* block,
    uint32_t idx,
    char* buffer,
    variant_t* variants,
    uint32_t variant_count,
    arena_t* scratch_arena)
{
    size_t if_start_idx = 0;
    size_t line_idx = 0;
    // If true, signifies an if a statement was found, but condition not met. So we are searching
    // for a matching pre-processor statement.
    bool searching = false;
    bool error;

    while (idx < block->len)
    {
        string_t line = read_line(block->data, &idx, block->len, buffer);
        ++line_idx;
        // Check for nested pre-processor if statement.
        if (string_contains(&line, "#if"))
        {
            if_start_idx = idx - line.len;
            if (pp_parse_if(&line, variants, variant_count, scratch_arena, line_idx, &error))
            {
                string_t new_block;
                if (!pp_edit_shader_block(
                        block, &idx, buffer, if_start_idx, scratch_arena, &new_block))
                {
                    return false;
                }
                // Update the block - we won't worry about de-allocating as everything is done in
                // the scratch arena for now.
                block->data = new_block.data;
                block->len = new_block.len;
                continue;
            }
            searching = true;
        }
        else if (string_contains(&line, "#elif"))
        {
            // If we have hit a #elif statement but found no matching if statement, then this is an
            // error.
            if (!searching)
            {
                log_error("Missing #if statement for #elif at line %i", line_idx);
                return false;
            }
            if (pp_parse_if(&line, variants, variant_count, scratch_arena, line_idx, &error))
            {
                string_t new_block;
                if (!pp_edit_shader_block(
                        block, &idx, buffer, if_start_idx, scratch_arena, &new_block))
                {
                    return false;
                }
                // Update the block - we won't worry about de-allocating as everything is done in
                // the scratch arena for now.
                block->data = new_block.data;
                block->len = new_block.len;
                searching = false;
                continue;
            }
        }
        else if (string_contains(&line, "#else"))
        {
            if (!searching)
            {
                log_error("Missing #if statement for #else at line %i", line_idx);
                return false;
            }
            // Remove all text from the #if to #else statements.
            string_t new_block = string_remove(block, if_start_idx, idx - 1, scratch_arena);
            idx -= idx - if_start_idx;

            // find #endif which denotes the end of the #else code block.
            uint32_t end_idx = UINT32_MAX;
            uint32_t end_sz;

            while (idx < block->len)
            {
                line = read_line(new_block.data, &idx, new_block.len, buffer);
                if (string_contains(&line, "#endif"))
                {
                    end_idx = idx - line.len;
                    end_sz = line.len - 1;
                    break;
                }
            }
            if (end_idx == UINT32_MAX)
            {
                log_error("Incorrectly terminated #if block: missing #endif statement.");
                return false;
            }
            // Remove code around else block.
            new_block = string_remove(&new_block, end_idx, end_idx + end_sz, scratch_arena);
            block->data = new_block.data;
            block->len = new_block.len;
            searching = false;
        }
        else if (string_contains(&line, "#endif"))
        {
            if (!searching)
            {
                log_error("Missing #if statement for #endif at line %i", line_idx);
                return false;
            }

            // We hit this if we have a single definition that evaluates to false. So remove the
            // whole block.
            assert(if_start_idx < idx);
            string_t new_block = string_remove(block, if_start_idx, idx - 1, scratch_arena);
            block->data = new_block.data;
            block->len = new_block.len;
            idx -= idx - if_start_idx;
            searching = false;
        }
    }
    return true;
}

string_t shader_program_process_preprocessor(
    string_t* block,
    variant_t* variants,
    uint32_t variant_count,
    arena_t* arena,
    arena_t* scratch_arena)
{
    string_t out;
    out.data = NULL;
    out.len = 0;

    int idx = 0;
    char buffer[100];
    if (!pp_parse_preprocessor_branch(block, idx, buffer, variants, variant_count, scratch_arena))
    {
        return out;
    }
    // As we created the shader code block in the scratch arena, now editing is complete, transfer
    // to the lifetime arena.
    out.len = block->len;
    out.data = arena_alloc(arena, sizeof(char), _Alignof(char), out.len + 1, 0);
    memcpy(out.data, block->data, out.len);
    out.data[out.len] = '\0';

    arena_reset(scratch_arena);
    return out;
}