#include "string.h"

#include "arena.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

string_t string_init(const char* str, arena_t* arena)
{
    assert(str);

    string_t s;
    s.len = strlen(str);
    s.data = arena_alloc(arena, s.len + 1, _Alignof(char), 1, 0);
    s.data[s.len - 1] = '\0';
    strcpy(s.data, str);
    return s;
}

string_t string_copy(string_t* other, arena_t* arena)
{
    assert(other);
    assert(other->data);
    return string_init(other->data, arena);
}

bool string_cmp(string_t* a, string_t* b)
{
    assert(a);
    assert(b);
    return strcmp(a->data, b->data) == 0;
}

string_t string_substring(string_t* s, uint32_t start, uint32_t end, arena_t* arena)
{
    assert(s);
    assert(start >= 0);
    assert(end >= start);
    assert(end < s->len);

    string_t sub;
    sub.len = end - start + 1;
    sub.data = arena_alloc(arena, 1, _Alignof(char), sub.len + 1, 0);

    strncpy(sub.data, s->data + start, sub.len);
    sub.data[sub.len] = '\0';
    return sub;
}

char* string_contains(string_t* s, const char* sub)
{
    assert(s);
    assert(sub);
    assert(s->len > 0);
    return strstr(s->data, sub);
}

string_t** string_split(string_t* s, char literal, uint32_t* count, arena_t* arena)
{
    assert(s);

    *count = 0;
    for (const char* p = s->data; (p = strchr(p, literal)) != NULL; ++p)
    {
        ++(*count);
    }
    // If no splitting occurs, just return the original string (count = 1).
    if (!(*count))
    {
        string_t** strs = ARENA_MAKE_ARRAY(arena, string_t*, 1, 0);
        strs[0] = ARENA_MAKE_STRUCT(arena, string_t, 0);
        strs[0]->len = s->len;
        strs[0]->data = arena_alloc(arena, strs[0]->len + 1, _Alignof(char), 1, 0);
        strcpy(strs[0]->data, s->data);
        (*count) = 1;
        return strs;
    }

    // Note: We overshoot on the memory allocation here by one if the string literal to split on
    // occurs at the beginning or end of that string.
    string_t** strs = ARENA_MAKE_ARRAY(arena, string_t*, *count + 1, 0);

    int c = 0;
    const char* curr = s->data;
    for (const char* p = s->data; (p = strchr(p, literal)) != NULL; ++p)
    {
        if (p == s->data)
        {
            // Adjust the count to take into account that the split is occurring at the beginning of
            // the string, hence there will be no left-hand string.
            --(*count);
            curr = p + 1;
            continue;
        }
        strs[c] = ARENA_MAKE_STRUCT(arena, string_t, 0);
        strs[c]->len = (p)-curr;
        strs[c]->data = arena_alloc(arena, sizeof(char), _Alignof(char), strs[c]->len + 1, 0);

        strncpy(strs[c]->data, curr, strs[c]->len);
        strs[c]->data[strs[c]->len] = '\0';
        curr = p + 1;
        ++c;
    }

    if (curr != s->data + s->len)
    {
        strs[c] = ARENA_MAKE_STRUCT(arena, string_t, 0);
        strs[c]->len = (s->data + s->len) - curr;
        strs[c]->data = arena_alloc(arena, sizeof(char), _Alignof(char), strs[c]->len + 1, 0);
        strcpy(strs[c]->data, curr);
        strs[c]->data[strs[c]->len] = '\0';
        ++(*count);
    }

    return strs;
}

string_t string_append(string_t* a, const char* b, arena_t* arena)
{
    assert(a);
    assert(b);

    string_t new_str;

    if (!a->data)
    {
        new_str.len = strlen(b);
        new_str.data = arena_alloc(arena, sizeof(char), _Alignof(char), new_str.len + 1, 0);
        strcpy(new_str.data, b);
        return new_str;
    }

    new_str.len = a->len + strlen(b);
    new_str.data = arena_alloc(arena, sizeof(char), _Alignof(char), new_str.len + 1, 0);

    assert(new_str.data);

    strcpy(new_str.data, a->data);
    strcat(new_str.data, b);
    new_str.data[new_str.len] = '\0';

    return new_str;
}

string_t string_append3(string_t* a, const char* b, const char* c, arena_t* arena)
{
    assert(a);
    assert(b);
    assert(c);

    string_t s1 = string_append(a, b, arena);
    string_t s2 = string_append(&s1, c, arena);
    return s2;
}

uint32_t string_find_first_of(string_t* str, char c)
{
    assert(str);
    uint32_t out = UINT32_MAX;
    for (uint32_t i = 0; i < str->len; ++i)
    {
        if (str->data[i] == c)
        {
            out = i;
            break;
        }
    }
    return out;
}

uint32_t string_find_last_of(string_t* str, char c)
{
    assert(str);
    uint32_t out = UINT32_MAX;
    if (!str->len)
    {
        return out;
    }

    for (uint32_t i = str->len - 1; i > 0; --i)
    {
        if (str->data[i] == c)
        {
            out = i;
            break;
        }
    }
    return out;
}

uint32_t string_count(string_t* str, const char* cs)
{
    assert(str);

    size_t sz = strlen(cs);
    assert(sz > 0);

    uint32_t count = 0;

    string_t curr_data;
    curr_data.data = str->data;
    curr_data.len = str->len;

    for (;;)
    {
        uint32_t first_idx = string_find_first_of(&curr_data, cs[0]);
        if (first_idx == UINT32_MAX || first_idx + sz > curr_data.len)
        {
            break;
        }
        if (strncmp(curr_data.data + first_idx, cs, sz) == 0)
        {
            ++count;
        }
        curr_data.data += first_idx + sz;
        curr_data.len -= first_idx + sz;
    }
    return count;
}

string_t string_remove(string_t* str, uint32_t start_idx, uint32_t end_idx, arena_t* arena)
{
    assert(str);
    assert(start_idx <= end_idx);
    assert(end_idx < str->len);

    string_t out;
    uint32_t sz = end_idx - start_idx;
    if (!sz)
    {
        out.data = str->data;
        out.len = str->len;
        return out;
    }
    out.len = str->len - sz - 1;
    out.data = arena_alloc(arena, sizeof(char), _Alignof(char), out.len + 1, 0);

    strncpy(out.data, str->data, start_idx);
    strncpy(out.data + start_idx, str->data + end_idx + 1, str->len - end_idx - 1);
    out.data[out.len] = '\0';
    return out;
}

string_t string_trim(string_t* str, char c, arena_t* arena)
{
    assert(str);
    string_t out;
    out.data = arena_alloc(arena, sizeof(char), _Alignof(char), str->len + 1, 0);
    out.len = 0;

    for (uint32_t i = 0; i < str->len; ++i)
    {
        if (str->data[i] != c)
        {
            out.data[out.len++] = str->data[i];
        }
    }
    out.data[out.len] = '\0';
    return out;
}

string_t string_replace(string_t* str, const char* orig, const char* rep, arena_t* arena)
{
    assert(str);
    assert(orig);
    assert(rep);
#
    string_t out;

    uint32_t orig_count = string_count(str, orig);
    if (!orig_count)
    {
        out.data = str->data;
        out.len = str->len;
        return out;
    }

    size_t orig_sz = strlen(orig);
    size_t rep_sz = strlen(rep);
    assert(orig_sz && rep_sz);

    out.len = (str->len - (orig_sz * orig_count)) + (rep_sz * orig_count);
    out.data = arena_alloc(arena, sizeof(char), _Alignof(char), out.len + 1, 0);
    assert(out.data);

    // A copy of the output pointer and length.
    string_t curr_out = {.data = out.data, .len = out.len};
    // Copy the original string so the pointer is preserved.
    string_t curr_orig = {.data = str->data, .len = str->len};

    for (;;)
    {
        uint32_t first_idx = string_find_first_of(&curr_orig, orig[0]);
        if (first_idx == UINT32_MAX || first_idx + orig_sz > curr_out.len)
        {
            // Copy and remaining chars before finishing (no more replacement chars).
            strcpy(curr_out.data, curr_orig.data);
            break;
        }
        if (strncmp(curr_orig.data + first_idx, orig, orig_sz) == 0)
        {
            // Copy string to the point of the matched characters to replace.
            strncpy(curr_out.data, curr_orig.data, first_idx);
            // Copy the replacement string.
            strncpy(curr_out.data + first_idx, rep, rep_sz);

            curr_out.data += first_idx + rep_sz;
            curr_out.len -= first_idx + rep_sz - 1;
            curr_orig.data += first_idx + orig_sz;
            curr_orig.len -= first_idx + orig_sz - 1;
        }
        else
        {
            // Not an exact match so just copy upto the index.
            strncpy(curr_out.data, curr_orig.data, first_idx + 1);

            curr_out.data += first_idx + 1;
            curr_out.len -= first_idx + 1;
            curr_orig.data += first_idx + 1;
            curr_orig.len -= first_idx + 1;
        }
    }
    out.data[out.len] = '\0';

    return out;
}