#include "vk_setup.h"

#include <rpe/engine.h>
#include <rpe/settings.h>
#include <string.h>
#include <unity_fixture.h>

TEST_GROUP(EngineGroup);

TEST_SETUP(EngineGroup) {}

TEST_TEAR_DOWN(EngineGroup) {}

TEST(EngineGroup, General_Test)
{
    vkapi_driver_t* driver = setup_driver();

    rpe_settings_t settings = {.draw_shadows = false, .gbuffer_dims = 1024};
    rpe_engine_t* engine = rpe_engine_create(driver, &settings);
    CHECK(engine);

    rpe_engine_shutdown(engine);
}