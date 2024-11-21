
#include <assert.h>
#include <vulkan-api/driver.h>
#include <vulkan-api/error_codes.h>

static inline vkapi_driver_t* setup_driver()
{
    vkapi_driver_t* driver;
    int error_code;
    driver = vkapi_driver_init(NULL, 0, &error_code);
    assert(error_code == VKAPI_SUCCESS);
    error_code = vkapi_driver_create_device(driver, NULL);
    assert(error_code == VKAPI_SUCCESS);
    return driver;
}

static inline arena_t* setup_arena(size_t size)
{
    arena_t* arena = calloc(1, sizeof(arena_t));
    int res = arena_new(size, arena);
    assert(ARENA_SUCCESS == res);
    return arena;
}

static inline void test_shutdown(vkapi_driver_t* driver, arena_t* arena)
{
    vkapi_driver_shutdown(driver);
    arena_release(arena);
    free(arena);
}
