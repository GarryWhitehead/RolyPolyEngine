#include "unity.h"
#include "unity_fixture.h"
#include "utility/arena.h"
#include "utility/hash.h"
#include "utility/hash_set.h"
#include "utility/random.h"

TEST_GROUP(HashSetGroup);

TEST_SETUP(HashSetGroup) {}

TEST_TEAR_DOWN(HashSetGroup) {}

TEST(HashSetGroup, HashSet_GeneralTests)
{
    arena_t arena;
    uint64_t arena_cap = 1 << 15;
    int res = arena_new(arena_cap, &arena);
    TEST_ASSERT(ARENA_SUCCESS == res);

    hash_set_t set = HASH_SET_CREATE(int, float, &arena);
    TEST_ASSERT(set.size == 0);

    float vals[] = {1.0f, 2.0f, 4.0f, 10.0f};
    int keys[] = {10, 20, 40, 100};
    HASH_SET_INSERT(&set, &keys[0], &vals[0]);
    TEST_ASSERT(set.size == 1);

    float* ret = HASH_SET_GET(&set, &keys[0]);
    TEST_ASSERT(ret);
    TEST_ASSERT_EQUAL_INT(vals[0], *ret);

    HASH_SET_INSERT(&set, &keys[1], &vals[1]);
    HASH_SET_INSERT(&set, &keys[2], &vals[2]);
    HASH_SET_INSERT(&set, &keys[3], &vals[3]);

    TEST_ASSERT(HASH_SET_FIND(&set, &keys[2]));
    TEST_ASSERT(HASH_SET_ERASE(&set, &keys[2]));
    TEST_ASSERT_FALSE(HASH_SET_FIND(&set, &keys[2]));
    TEST_ASSERT(set.size == 3);

    HASH_SET_INSERT(&set, &keys[2], &vals[2]);
    TEST_ASSERT_TRUE(HASH_SET_FIND(&set, &keys[2]));
    TEST_ASSERT(set.size == 4);

    float new_val = 88.8f;
    HASH_SET_SET(&set, &keys[2], &new_val);
    ret = HASH_SET_GET(&set, &keys[2]);
    TEST_ASSERT_NOT_NULL(ret);
    TEST_ASSERT(*ret == new_val);
}

TEST(HashSetGroup, HashSet_ResizeTests)
{
    arena_t arena;
    uint64_t arena_cap = 1 << 25;
    int res = arena_new(arena_cap, &arena);
    TEST_ASSERT(ARENA_SUCCESS == res);

    hash_set_t set = HASH_SET_CREATE(int, int, &arena);
    xoro_rand_t rand = xoro_rand_init(0xff, 0x1234);

    for (int i = 0; i < 1000; ++i)
    {
        int val = (int)xoro_rand_next(&rand);
        HASH_SET_INSERT(&set, &i, &val);
        int* res = HASH_SET_GET(&set, &i);
        TEST_ASSERT_NOT_NULL(res);
        TEST_ASSERT_EQUAL(val, *res);
    }
}

TEST(HashSetGroup, HashSet_IteratorTests)
{
    arena_t arena;
    uint64_t arena_cap = 1 << 25;
    int res = arena_new(arena_cap, &arena);
    TEST_ASSERT(ARENA_SUCCESS == res);

    hash_set_t set = HASH_SET_CREATE(int, int, &arena);
    hash_set_iterator_t it = hash_set_iter_create(&set);
    int* ret = hash_set_iter_next(&it);
    TEST_ASSERT(ret == NULL);

    // NOTE: unordered map so when iterating not necessarily in order.
    int key = 0;
    int val = 1;
    HASH_SET_INSERT(&set, &key, &val);
    ++key;
    ++val;
    HASH_SET_INSERT(&set, &key, &val);
    key += 20;
    val += 20;
    HASH_SET_INSERT(&set, &key, &val);
    it = hash_set_iter_create(&set);
    ret = hash_set_iter_next(&it);
    TEST_ASSERT(ret != NULL);
    TEST_ASSERT_EQUAL_INT(2, *ret);
    ret = hash_set_iter_next(&it);
    TEST_ASSERT(ret != NULL);
    TEST_ASSERT_EQUAL_INT(22, *ret);
    ret = hash_set_iter_next(&it);
    TEST_ASSERT(ret != NULL);
    TEST_ASSERT_EQUAL_INT(1, *ret);
    ret = hash_set_iter_next(&it);
    TEST_ASSERT(ret == NULL);

    // Test erasure using an iterator.
    it = hash_set_iter_create(&set);
    hash_set_iter_next(&it);
    hash_set_iter_next(&it);
    hash_set_iter_erase(&it);
    int find_key = 2;
    TEST_ASSERT(hash_set_find(&set, &find_key) == false);
    find_key = 1;
    TEST_ASSERT(hash_set_find(&set, &find_key) == true);
    hash_set_iter_next(&it);
    hash_set_iter_erase(&it);
    TEST_ASSERT(hash_set_find(&set, &find_key) == true);
    TEST_ASSERT_EQUAL_UINT(1, set.size);
}