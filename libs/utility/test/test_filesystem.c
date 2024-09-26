#include "unity.h"
#include "unity_fixture.h"
#include "utility/filesystem.h"
#include "utility/string.h"
#include "utility/arena.h"

#include <string.h>

TEST_GROUP(FilesystemGroup);

TEST_SETUP(FilesystemGroup) {}

TEST_TEAR_DOWN(FilesystemGroup) {}

TEST(FilesystemGroup, Filesystem_Extension)
{
    arena_t arena;
    uint64_t arena_cap = 1 << 25;
    int res = arena_new(arena_cap, &arena);
    TEST_ASSERT(ARENA_SUCCESS == res);

    string_t path = string_init("/path/to/somewhere/file.cpp", &arena);
    string_t ext;
    bool r = fs_get_extension(&path, &ext, &arena);
    TEST_ASSERT(r == true);
    TEST_ASSERT_EQUAL_STRING("cpp", ext.data);

    path = string_init("/path/to/somewhere/file.h", &arena);
    r = fs_get_extension(&path, &ext, &arena);
    TEST_ASSERT(r == true);
    TEST_ASSERT_EQUAL_STRING("h", ext.data);
}