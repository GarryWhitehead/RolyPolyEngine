#include "unity.h"
#include "unity_fixture.h"
#include "utility/sort.h"

TEST_GROUP(SortGroup);

TEST_SETUP(SortGroup) {}

TEST_TEAR_DOWN(SortGroup) {}

TEST(SortGroup, RadixSortTest)
{
    arena_t arena;
    uint64_t arena_cap = 1 << 15;
    int res = arena_new(arena_cap, &arena);
    TEST_ASSERT(ARENA_SUCCESS == res);

    uint64_t arr1[] = {3, 2, 1, 0};
    uint64_t out1[4];
    radix_sort(arr1, 4, &arena, out1);
    TEST_ASSERT_EQUAL_UINT(3, out1[0]);
    TEST_ASSERT_EQUAL_UINT(2, out1[1]);
    TEST_ASSERT_EQUAL_UINT(1, out1[2]);
    TEST_ASSERT_EQUAL_UINT(0, out1[3]);

    uint64_t arr2[] = {1, 0, 3, 90, 6, 8, 5, 101, 4, 2, 10, 9, 200};
    uint64_t out2[13];
    uint64_t expected2[] = {1, 0, 9, 2, 8, 6, 4, 5, 11, 10, 3, 7, 12};
    radix_sort(arr2, 13, &arena, out2);
    for (int i = 0; i < 13; ++i)
    {
        TEST_ASSERT_EQUAL_UINT(expected2[i], out2[i]);
    }

    // Larger numbers this time.
    uint64_t arr3[] = {10000, 100, 5, 20, 99, 4449991};
    uint64_t out3[6];
    uint64_t expected3[] = {2, 3, 4, 1, 0, 5};
    radix_sort(arr3, 6, &arena, out3);
    for (int i = 0; i < 6; ++i)
    {
        TEST_ASSERT_EQUAL_UINT(expected3[i], out3[i]);
    }

    // No sorting required
    uint64_t arr4[] = {10, 100, 500, 6000, 800000, 100000000};
    uint64_t out4[6];
    uint64_t expected4[] = {0, 1, 2, 3, 4, 5};
    radix_sort(arr4, 6, &arena, out4);
    for (int i = 0; i < 6; ++i)
    {
        TEST_ASSERT_EQUAL_UINT(expected4[i], out4[i]);
    }
}