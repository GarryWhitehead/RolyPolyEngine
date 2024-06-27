#include "unity.h"
#include "unity_fixture.h"

#include <utility/array.h>

TEST_GROUP(ArrayGroup);

TEST_SETUP(ArrayGroup) {}

TEST_TEAR_DOWN(ArrayGroup) {}

TEST(ArrayGroup, GenericTests)
{
    const uint32_t array_size = 10;
    array_t array = ARRAY_INIT(int, array_size);

    TEST_ASSERT_EQUAL_UINT32(array.size, array_size);
    TEST_ASSERT_TRUE(array.data);

    int num = 20;
    ARRAY_ASSIGN(int, &array, 5, &num);
    int out = ARRAY_GET(int, &array, 5);
    TEST_ASSERT_EQUAL_INT(num, out);

    num = 0;
    ARRAY_FOR_EACH(int, *val, array) { *val = num++; }

    int res1 = ARRAY_GET(int, &array, 1);
    int res2 = ARRAY_GET(int, &array, 9);
    TEST_ASSERT_EQUAL_INT(res1, 1);
    TEST_ASSERT_EQUAL_INT(res2, 9);

    array_clear(&array);
    res1 = ARRAY_GET(int, &array, 1);
    res2 = ARRAY_GET(int, &array, 9);
    TEST_ASSERT_EQUAL_INT(res1, 0);
    TEST_ASSERT_EQUAL_INT(res2, 0);

    array_free(&array);
}
