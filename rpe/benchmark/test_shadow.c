#include <utility/benchmark.h>

#include <shadow_manager.h>
#include <scene.h>
#include <camera.h>
#include <engine.h>
#include <managers/light_manager.h>
#include <rpe/light_manager.h>
#include <rpe/object_manager.h>
#include <rpe/engine.h>
#include <vulkan-api/error_codes.h>
#include <log.h>

void BM_test_shadow_cascade_gen(bm_run_state_t* state)
{
    log_set_quiet(true);
    int64_t cascade_count = state->arg;

    rpe_shadow_manager_t sm;
    sm.settings.cascade_count = (int)cascade_count;
    sm.settings.split_lambda = 0.9f;

    rpe_scene_t scene;
    rpe_camera_t camera = {.n = 0.1f, .z = 100.0f };

    while (bm_state_set_running(state))
    {
        rpe_shadow_manager_compute_csm_splits(&sm, &scene, &camera);
    }
}

BENCHMARK_ARG3(BM_test_shadow_cascade_gen, 2, 5, 8)

void BM_test_shadow_update_projection(bm_run_state_t* state)
{
    log_set_quiet(true);
    int64_t cascade_count = state->arg;

    vkapi_driver_t* driver;
    int error_code;
    driver = vkapi_driver_init(NULL, 0, &error_code);
    assert(error_code == VKAPI_SUCCESS);
    error_code = vkapi_driver_create_device(driver, NULL);
    assert(error_code == VKAPI_SUCCESS);

    rpe_settings_t settings = {0};
    rpe_engine_t* engine = rpe_engine_create(driver, &settings);

    struct ShadowSettings sm_settings;
    sm_settings.cascade_count = (int)cascade_count;
    sm_settings.split_lambda = 0.9f;
    rpe_shadow_manager_t* sm = rpe_shadow_manager_init(engine, sm_settings);

    rpe_scene_t scene;
    rpe_camera_t camera = {.n = 0.1f, .z = 100.0f };

    rpe_light_manager_t* lm = rpe_light_manager_init(engine);
    rpe_light_create_info_t ci = {.position = {0.0f, -5.0f, 1.0f}};
    rpe_object_t obj = {.id = 1};
    rpe_light_manager_create_light(lm, &ci, obj, RPE_LIGHTING_TYPE_DIRECTIONAL);

    while (bm_state_set_running(state))
    {
        rpe_shadow_manager_update_projections(sm, &camera, &scene, engine, lm);
    }
}

BENCHMARK_ARG3(BM_test_shadow_update_projection, 2, 5, 8);

BENCHMARK_MAIN()