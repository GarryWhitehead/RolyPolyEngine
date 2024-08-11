#include "unity.h"
#include "unity_fixture.h"
#include "utility/arena.h"
#include "utility/string.h"

#include <string.h>

TEST_GROUP(StringGroup);

TEST_SETUP(StringGroup) {}

TEST_TEAR_DOWN(StringGroup) {}

TEST(StringGroup, StringTests_General)
{
    arena_t arena;
    uint64_t arena_cap = 1 << 25;
    int res = arena_new(arena_cap, &arena);
    TEST_ASSERT(ARENA_SUCCESS == res);

    string_t str = string_init("I am a test string", &arena);
    TEST_ASSERT(str.data);
    TEST_ASSERT(str.len == 18);
    TEST_ASSERT(str.arena);

    string_t sub_str = string_substring(&str, 7, 10);
    TEST_ASSERT(sub_str.data);
    TEST_ASSERT(sub_str.len == 4);
    TEST_ASSERT(strcmp(sub_str.data, "test") == 0);

    TEST_ASSERT(string_contains(&str, "string") != NULL);
    TEST_ASSERT(string_contains(&str, "invalid") == NULL);

    uint32_t count;
    string_t** splits = string_split(&str, ' ', &count);
    TEST_ASSERT(splits);
    TEST_ASSERT(count == 5);
    TEST_ASSERT(strcmp(splits[0]->data, "I") == 0);
    TEST_ASSERT(strcmp(splits[1]->data, "am") == 0);
    TEST_ASSERT(strcmp(splits[2]->data, "a") == 0);
    TEST_ASSERT(strcmp(splits[3]->data, "test") == 0);
    TEST_ASSERT(strcmp(splits[4]->data, "string") == 0);

    splits = string_split(&str, 'I', &count);
    TEST_ASSERT(count == 1);
    TEST_ASSERT(strcmp(splits[0]->data, " am a test string") == 0);

    splits = string_split(&str, 'g', &count);
    TEST_ASSERT(count == 1);
    TEST_ASSERT(strcmp(splits[0]->data, "I am a test strin") == 0);

    TEST_ASSERT(string_split(&str, '<', &count) == NULL);

    TEST_ASSERT(string_cmp(&str, &str) == true);
    TEST_ASSERT(string_cmp(&str, splits[0]) == false);

    arena_release(&arena);
}