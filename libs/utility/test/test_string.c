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

    string_t sub_str = string_substring(&str, 7, 10, &arena);
    TEST_ASSERT(sub_str.data);
    TEST_ASSERT(sub_str.len == 4);
    TEST_ASSERT(strcmp(sub_str.data, "test") == 0);

    TEST_ASSERT(string_contains(&str, "string") != NULL);
    TEST_ASSERT(string_contains(&str, "invalid") == NULL);

    uint32_t count;
    string_t** splits = string_split(&str, ' ', &count, &arena);
    TEST_ASSERT(splits);
    TEST_ASSERT(count == 5);
    TEST_ASSERT(strcmp(splits[0]->data, "I") == 0);
    TEST_ASSERT(strcmp(splits[1]->data, "am") == 0);
    TEST_ASSERT(strcmp(splits[2]->data, "a") == 0);
    TEST_ASSERT(strcmp(splits[3]->data, "test") == 0);
    TEST_ASSERT_EQUAL_STRING("string", splits[4]->data);
    TEST_ASSERT_EQUAL_UINT(1, splits[0]->len);
    TEST_ASSERT_EQUAL_UINT(2, splits[1]->len);
    TEST_ASSERT_EQUAL_UINT(1, splits[2]->len);
    TEST_ASSERT_EQUAL_UINT(4, splits[3]->len);
    TEST_ASSERT_EQUAL_UINT(6, splits[4]->len);

    splits = string_split(&str, 'I', &count, &arena);
    TEST_ASSERT(count == 1);
    TEST_ASSERT(strcmp(splits[0]->data, " am a test string") == 0);
    TEST_ASSERT_EQUAL_UINT(17, splits[0]->len);

    splits = string_split(&str, 'g', &count, &arena);
    TEST_ASSERT(count == 1);
    TEST_ASSERT(strcmp(splits[0]->data, "I am a test strin") == 0);
    TEST_ASSERT_EQUAL_UINT(17, splits[0]->len);

    splits = string_split(&str, '<', &count, &arena);
    TEST_ASSERT(count == 1);
    TEST_ASSERT_EQUAL_STRING(str.data, splits[0]->data);

    string_t invalid_str = string_init("I am a invalid string", &arena);
    TEST_ASSERT(string_cmp(&str, &str) == true);
    TEST_ASSERT(string_cmp(&str, &invalid_str) == false);

    string_t trimmed = string_trim(&invalid_str, ' ', &arena);
    TEST_ASSERT(strcmp(trimmed.data, "Iamainvalidstring") == 0);
    TEST_ASSERT_EQUAL_UINT(17, trimmed.len);

    uint32_t first_idx = string_find_first_of(&invalid_str, 'a');
    TEST_ASSERT_EQUAL_UINT(2, first_idx);
    first_idx = string_find_first_of(&invalid_str, 'z');
    TEST_ASSERT_EQUAL_UINT(UINT32_MAX, first_idx);

    uint32_t last_idx = string_find_last_of(&invalid_str, 'i');
    TEST_ASSERT_EQUAL_UINT(18, last_idx);
    last_idx = string_find_last_of(&invalid_str, 'z');
    TEST_ASSERT_EQUAL_UINT(UINT32_MAX, last_idx);

    uint32_t letter_count = string_count(&invalid_str, "a");
    TEST_ASSERT_EQUAL_UINT(3, letter_count);
    letter_count = string_count(&invalid_str, "z");
    TEST_ASSERT_EQUAL_UINT(0, letter_count);
    string_t repeat_str = string_init("1 && 2 && 3 && 4 & 5 & 7&&", &arena);
    letter_count = string_count(&repeat_str, "&&");
    TEST_ASSERT_EQUAL_UINT(4, letter_count);
    letter_count = string_count(&repeat_str, "&");
    TEST_ASSERT_EQUAL_UINT(10, letter_count);

    string_t replace_str = string_replace(&repeat_str, "&&", "||", &arena);
    TEST_ASSERT_EQUAL_STRING("1 || 2 || 3 || 4 & 5 & 7||", replace_str.data);
    TEST_ASSERT_EQUAL_UINT(26, replace_str.len);

    replace_str = string_replace(&repeat_str, "&", "||", &arena);
    TEST_ASSERT_EQUAL_STRING("1 |||| 2 |||| 3 |||| 4 || 5 || 7||||", replace_str.data);
    TEST_ASSERT_EQUAL_UINT(36, replace_str.len);

    replace_str = string_replace(&repeat_str, "&&", "FooBar", &arena);
    TEST_ASSERT_EQUAL_STRING("1 FooBar 2 FooBar 3 FooBar 4 & 5 & 7FooBar", replace_str.data);
    TEST_ASSERT_EQUAL_UINT(42, replace_str.len);

    replace_str = string_replace(&repeat_str, "&&", "|", &arena);
    TEST_ASSERT_EQUAL_STRING("1 | 2 | 3 | 4 & 5 & 7|", replace_str.data);
    TEST_ASSERT_EQUAL_UINT(22, replace_str.len);

    // Test replacing with longer char runs between replacement strings.
    string_t repeat_str2 = string_init(
        "I am a great program && I am a great program && I am a even better program && Whatever",
        &arena);
    replace_str = string_replace(&repeat_str2, "&&", "&", &arena);
    TEST_ASSERT_EQUAL_STRING(
        "I am a great program & I am a great program & I am a even better program & Whatever",
        replace_str.data);

    string_t edit_str = string_remove(&invalid_str, 6, 14, &arena);
    TEST_ASSERT_EQUAL_STRING("I am astring", edit_str.data);
    TEST_ASSERT_EQUAL_UINT(12, edit_str.len);
    edit_str = string_remove(&invalid_str, 14, 20, &arena);
    TEST_ASSERT_EQUAL_STRING("I am a invalid", edit_str.data);
    TEST_ASSERT_EQUAL_UINT(14, edit_str.len);
    edit_str = string_remove(&invalid_str, 0, 14, &arena);
    TEST_ASSERT_EQUAL_STRING("string", edit_str.data);
    TEST_ASSERT_EQUAL_UINT(6, edit_str.len);
    edit_str = string_remove(&invalid_str, 5, 5, &arena);
    TEST_ASSERT_EQUAL_STRING("I am a invalid string", edit_str.data);

    string_t test_line1 = string_init(
        "#if (defined(TEST_DEF1) && defined(TEST_DEF2)) || (defined(TEST_DEP3) && "
        "defined(TEST_DEP4))",
        &arena);
    string_t new_line = string_substring(
        &test_line1, string_find_first_of(&test_line1, ' ') + 1, test_line1.len - 1, &arena);
    trimmed = string_trim(&new_line, ' ', &arena);
    replace_str = string_replace(&trimmed, "||", "|", &arena);
    splits = string_split(&replace_str, '|', &count, &arena);
    TEST_ASSERT_EQUAL_UINT(2, count);
    TEST_ASSERT_EQUAL_UINT(40, splits[0]->len);
    TEST_ASSERT_EQUAL_UINT(40, splits[1]->len);

    arena_release(&arena);
}