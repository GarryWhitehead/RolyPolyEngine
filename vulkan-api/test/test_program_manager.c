#include <backend/enums.h>
#include <unity_fixture.h>
#include <vulkan-api/error_codes.h>
#include <vulkan-api/program_manager.h>

TEST_GROUP(ProgramManagerGroup);

TEST_SETUP(ProgramManagerGroup) {}

TEST_TEAR_DOWN(ProgramManagerGroup) {}

TEST(ProgramManagerGroup, PM_ShaderProgram_Tests)
{
    arena_t arena;
    uint64_t arena_cap = 1 << 20;
    int res = arena_new(arena_cap, &arena);
    TEST_ASSERT(ARENA_SUCCESS == res);

    arena_t scratch_arena;
    uint64_t scratch_arena_cap = 1 << 10;
    res = arena_new(arena_cap, &scratch_arena);
    TEST_ASSERT(ARENA_SUCCESS == res);

    int error_code;
    vkapi_driver_t* driver = vkapi_driver_init(NULL, 0, &error_code);
    TEST_ASSERT(error_code == VKAPI_SUCCESS);
    error_code = vkapi_driver_create_device(driver, NULL);
    TEST_ASSERT(error_code == VKAPI_SUCCESS);


    vkapi_driver_shutdown(driver, VK_NULL_HANDLE);
}