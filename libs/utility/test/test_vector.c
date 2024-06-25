#include "unity.h"
#include "unity_fixture.h"

#include <string.h>
#include <utility/vector.h>

TEST_GROUP(VectorGroup);

TEST_SETUP(VectorGroup) {}

TEST_TEAR_DOWN(VectorGroup) {}

TEST(VectorGroup, GenericTests)
{
    vector_t vec = VECTOR_INIT(int, 3);

    int items[] = {2, 4, 6, 8, 10};
    VECTOR_PUSH_BACK(int, &vec, &items[0]);
    TEST_ASSERT_EQUAL_UINT32(vec.size, 1);
    VECTOR_PUSH_BACK(int, &vec, &items[1]);
    TEST_ASSERT_EQUAL_UINT32(vec.size, 2);
    int item = VECTOR_POP_BACK(int, &vec);
    TEST_ASSERT_EQUAL_INT(item, items[1]);
    TEST_ASSERT_EQUAL_UINT32(vec.size, 1);

    VECTOR_PUSH_BACK(int, &vec, &items[1]);
    VECTOR_PUSH_BACK(int, &vec, &items[2]);
    // Should resize the memory here.
    VECTOR_PUSH_BACK(int, &vec, &items[3]);
    TEST_ASSERT_EQUAL_UINT32(vec.size, 4);
    VECTOR_PUSH_BACK(int, &vec, &items[4]);

    item = VECTOR_GET(int, &vec, 1);
    TEST_ASSERT_EQUAL_INT(item, items[1]);

    int new_val = 20;
    VECTOR_ASSIGN(int, &vec, 2, &new_val);
    item = VECTOR_GET(int, &vec, 2);
    TEST_ASSERT_EQUAL_INT(item, 20);

    // Set back to the initial value for the next set of tests.
    VECTOR_ASSIGN(int, &vec, 2, &items[2]);

    bool success = true;
    VECTOR_FOR_EACH(int, *val, vec) { success &= *val == items[count]; }

    vector_resize(&vec, 50);
    TEST_ASSERT_EQUAL_UINT32(vec.capacity, 50 * VEC_GROWTH_FACTOR);
    int res = memcmp(vec.data, items, sizeof(int) * 5);
    TEST_ASSERT(res == 0);

    TEST_ASSERT_FALSE(vector_is_empty(&vec));

    vector_clear(&vec);
    TEST_ASSERT_TRUE(vector_is_empty(&vec));
    TEST_ASSERT_EQUAL_UINT32(vec.capacity, 50 * VEC_GROWTH_FACTOR);
    TEST_ASSERT_EQUAL_UINT32(vec.size, 0);

    vector_free(&vec);
}
