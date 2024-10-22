#include "stdbool.h"
#include "string.h"
#include "unity.h"
#include "unity_fixture.h"

#include <utility/arena.h>

TEST_GROUP(ArenaGroup);

TEST_SETUP(ArenaGroup) {}

TEST_TEAR_DOWN(ArenaGroup) {}

TEST(ArenaGroup, ArenaTests_GeneralTests)
{
    arena_t arena;
    int err = arena_new(1 << 30, &arena);
    TEST_ASSERT_EQUAL(ARENA_SUCCESS, err);
    TEST_ASSERT_EQUAL_UINT32(arena.offset, 0);
    TEST_ASSERT_TRUE(arena.begin);

    int* alloc = ARENA_MAKE_ARRAY(&arena, int, 30, ARENA_ZERO_MEMORY);
    TEST_ASSERT_TRUE(alloc);
    TEST_ASSERT_EQUAL_UINT32(arena.offset, sizeof(int) * 30);

    bool success = true;
    for (int i = 0; i < 30; ++i)
    {
        success &= alloc[i] == 0;
    }
    TEST_ASSERT_TRUE(success);

    float* alloc1 = ARENA_MAKE_ZERO_STRUCT(&arena, float);
    TEST_ASSERT_TRUE(alloc1);
    TEST_ASSERT_EQUAL_UINT32(arena.offset, (sizeof(int) * 30) + (sizeof(float)));
    TEST_ASSERT(alloc1[0] == 0.0f);

    arena_reset(&arena);
    TEST_ASSERT_EQUAL_UINT32(arena.offset, 0);

    arena_release(&arena);
}

TEST(ArenaGroup, ArenaTests_DynamicArray)
{
    arena_t arena;
    int err = arena_new(1 << 30, &arena);
    TEST_ASSERT_EQUAL(ARENA_SUCCESS, err);

    arena_dyn_array_t array;
    err = MAKE_DYN_ARRAY(int, &arena, 3, &array);
    TEST_ASSERT_EQUAL(ARENA_SUCCESS, err);

    int vals[] = {1, 2, 3, 4, 5};
    DYN_ARRAY_APPEND(&array, &vals[0]);
    TEST_ASSERT_EQUAL_INT(DYN_ARRAY_GET(int, &array, 0), vals[0]);

    DYN_ARRAY_APPEND(&array, &vals[1]);
    DYN_ARRAY_APPEND(&array, &vals[2]);
    DYN_ARRAY_APPEND(&array, &vals[3]);
    DYN_ARRAY_APPEND(&array, &vals[4]);
    TEST_ASSERT_EQUAL_INT(DYN_ARRAY_GET(int, &array, 4), vals[4]);
    TEST_ASSERT_EQUAL_INT(array.size, 5);
    TEST_ASSERT_EQUAL_INT(array.capacity, 6);

    arena_release(&arena);
}

TEST(ArenaGroup, ArenaTests_DynamicArrayWithChar)
{
    arena_t arena;
    int err = arena_new(1 << 30, &arena);
    TEST_ASSERT_EQUAL(ARENA_SUCCESS, err);

    arena_dyn_array_t array;
    err = MAKE_DYN_ARRAY(char[100], &arena, 10, &array);
    TEST_ASSERT_EQUAL(ARENA_SUCCESS, err);

#define str1 "Hello from index 1.";
    const char* str2 = "Hello again.";
    DYN_ARRAY_APPEND_CHAR(&array, str1);
    DYN_ARRAY_APPEND_CHAR(&array, str2);

    const char* res1 = *DYN_ARRAY_GET_PTR(char*, &array, 0);
    TEST_ASSERT(strcmp(res1, "Hello from index 1.") == 0);
    const char* res2 = *DYN_ARRAY_GET_PTR(char*, &array, 1);
    TEST_ASSERT(strcmp(res2, str2) == 0);
}
