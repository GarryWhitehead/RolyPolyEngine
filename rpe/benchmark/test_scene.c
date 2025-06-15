#include <engine.h>
#include <managers/renderable_manager.h>
#include <managers/transform_manager.h>
#include <rpe/engine.h>
#include <rpe/material.h>
#include <rpe/object.h>
#include <rpe/object_manager.h>
#include <rpe/renderable_manager.h>
#include <rpe/transform_manager.h>
#include <scene.h>
#include <utility/benchmark.h>
#include <vulkan-api/error_codes.h>

void BM_test_upload_extents(bm_run_state_t* state)
{
    log_set_quiet(true);
    int64_t model_count = state->arg;

    vkapi_driver_t* driver;
    int error_code;
    driver = vkapi_driver_init(NULL, 0, &error_code);
    assert(error_code == VKAPI_SUCCESS);
    error_code = vkapi_driver_create_device(driver, NULL);
    assert(error_code == VKAPI_SUCCESS);

    rpe_settings_t settings = {0};
    rpe_engine_t* engine = rpe_engine_create(driver, &settings);

    rpe_scene_t* scene = rpe_engine_create_scene(engine);
    rpe_rend_manager_t* rm = rpe_engine_get_rend_manager(engine);
    rpe_transform_manager_t* tm = rpe_engine_get_transform_manager(engine);

    // Create a very simple mesh - just enough so we can run the benchmark...
    rpe_valloc_handle v_handle = rpe_rend_manager_alloc_vertex_buffer(rm, 1);
    rpe_valloc_handle i_handle = rpe_rend_manager_alloc_index_buffer(rm, 1);
    math_mat3f pos_data = {0.0f, 0.0f, 0.0f};
    uint16_t i_data = 0;
    rpe_mesh_t* mesh = rpe_rend_manager_create_static_mesh(
        rm,
        v_handle,
        (float*)&pos_data,
        NULL,
        NULL,
        NULL,
        1,
        i_handle,
        &i_data,
        1,
        RPE_RENDERABLE_INDICES_U16);

    rpe_model_transform_t mt = rpe_model_transform_init();
    rpe_object_t transform_obj = rpe_obj_manager_create_obj(rpe_engine_get_obj_manager(engine));
    rpe_transform_manager_add_local_transform(
        rpe_engine_get_transform_manager(engine), &mt, &transform_obj);
    rpe_transform_node_t node = {
        .world_transform = math_mat4f_identity(), .local_transform = math_mat4f_identity()};

    struct RenderableInstance* instances = malloc(sizeof(struct RenderableInstance) * model_count);
    for (int64_t i = 0; i < model_count; ++i)
    {
        rpe_object_t obj = rpe_obj_manager_create_obj(rpe_engine_get_obj_manager(engine));
        rpe_material_t* mat = rpe_rend_manager_create_material(rm, scene);
        rpe_renderable_t* rend = rpe_engine_create_renderable(engine, mat, mesh);
        rpe_rend_manager_add(rm, rend, obj, transform_obj);
        rpe_scene_add_object(scene, obj);
        instances[i].rend = rend;
        instances[i].transform = &node;
    }

    struct UploadExtentsEntry entry = {
        .scene = scene,
        .engine = engine,
        .tm = tm,
        .rm = rm,
        .instances = instances,
        .count = model_count};

    while (bm_state_set_running(state))
    {
        job_t* parent = job_queue_create_parent_job(engine->job_queue);
        rpe_scene_compute_model_extents(&entry, parent);
        job_queue_run_and_wait(engine->job_queue, parent);
    }
}

BENCHMARK_ARG3(BM_test_upload_extents, 100, 1000, 5000);