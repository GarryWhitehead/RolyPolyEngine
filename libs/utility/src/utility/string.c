#include "string.h"

#include "arena.h"

#include <assert.h>
#include <string.h>

string_t string_init(const char* str, arena_t* arena)
{
    assert(str);

    string_t s;
    s.len = strlen(str);
    s.data = arena_alloc(arena, s.len + 1, _Alignof(char), 1, 0);
    strcpy(s.data, str);
    s.arena = arena;
    return s;
}

string_t string_copy(string_t* other)
{
    assert(other);
    assert(other->data);
    return string_init(other->data, other->arena);
}

bool string_cmp(string_t* a, string_t* b)
{
    assert(a);
    assert(b);
    return strcmp(a->data, b->data) == 0;
}

string_t string_substring(string_t* s, uint32_t start, uint32_t end)
{
    assert(s);
    assert(end > start);
    assert(s->len >= end);

    string_t sub;
    sub.len = end - start + 1;
    sub.data = arena_alloc(s->arena, sub.len + 1, _Alignof(char), 1, 0);
    sub.arena = s->arena;
    strncpy(sub.data, s->data + start, sub.len);
    return sub;
}

char* string_contains(string_t* s, const char* sub)
{
    assert(s);
    assert(sub);
    assert(s->len > 0);
    return strstr(s->data, sub);
}

string_t** string_split(string_t* s, char literal, uint32_t* count)
{
    assert(s);

    *count = 0;
    for (const char* p = s->data; (p = strchr(p, literal)) != NULL; ++p)
    {
        ++(*count);
    }

    if (!(*count))
    {
        return NULL;
    }

    // Note: We overshoot on the memory allocation here by one if the string literal to split on
    // occurs at the beginning or end of that string.
    string_t** strs = ARENA_MAKE_ARRAY(s->arena, string_t*, *count + 1, 0);

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
        strs[c] = ARENA_MAKE_STRUCT(s->arena, string_t, 0);
        strs[c]->len = (p)-curr;
        strs[c]->data = arena_alloc(s->arena, strs[c]->len + 1, _Alignof(char), 1, 0);
        strs[c]->arena = s->arena;
        strncpy(strs[c]->data, curr, strs[c]->len);
        curr = p + 1;
        ++c;
    }

    if (curr != s->data + s->len)
    {
        strs[c] = ARENA_MAKE_STRUCT(s->arena, string_t, 0);
        strs[c]->len = (s->data + s->len) - curr;
        strs[c]->data = arena_alloc(s->arena, strs[c]->len + 1, _Alignof(char), 1, 0);
        strs[c]->arena = s->arena;
        strncpy(strs[c]->data, curr, strs[c]->len);
        ++(*count);
    }

    return strs;
}