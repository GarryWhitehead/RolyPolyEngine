#include "vk_setup.h"

#include <aabox.h>
#include <camera.h>
#include <compute.h>
#include <frustum.h>
#include <managers/renderable_manager.h>
#include <scene.h>
#include <unity_fixture.h>
#include <utility/maths.h>

TEST_GROUP(VisibilityGroup);

TEST_SETUP(VisibilityGroup) {}

TEST_TEAR_DOWN(VisibilityGroup) {}

TEST(VisibilityGroup, AABBox_Test)
{
    rpe_aabox_t box = {.min = {6.0f, 4.0f, 0.0f}, .max = {10.0f, 8.0f, 2.0f}};

    math_vec3f half = rpe_aabox_get_half_extent(&box);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, half.x);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, half.y);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, half.z);

    math_vec3f center = rpe_aabox_get_center(&box);
    TEST_ASSERT_EQUAL_FLOAT(8.0f, center.x);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, center.y);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, center.z);

    math_mat3f rot = math_mat3f_identity();
    math_vec3f t = {0.0f, 0.0f, 0.0f};
    rpe_aabox_t rigid_box = rpe_aabox_calc_rigid_transform(&box, rot, t);
    TEST_ASSERT_EQUAL_FLOAT(6.0f, rigid_box.min.x);
    TEST_ASSERT_EQUAL_FLOAT(4.0f, rigid_box.min.y);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, rigid_box.min.z);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, rigid_box.max.x);
    TEST_ASSERT_EQUAL_FLOAT(8.0f, rigid_box.max.y);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, rigid_box.max.z);
}

TEST(VisibilityGroup, VisCompute_Test)
{
    arena_t* arena = setup_arena(1 << 20);
    vkapi_driver_t* driver = setup_driver();

    math_mat4f proj = math_mat4f_frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 100.0f);
    rpe_frustum_t frustum = {0};
    rpe_frustum_projection(&frustum, &proj);

    math_vec4f half_extent = {0.5f, 0.5f, 0.5f, 0.0f};

    math_vec4f pass_translations[20] = {
        // box fully inside
        {0.0f, 0.0f, -10.0f, 0.0f},
        // box clipped by the near or far plane
        {0.0f, 0.0f, -1.0f, 0.0f},
        {0.0f, 0.0f, -100.0f, 0.0f},
        // box clipped by one or several planes of the frustum for any z, but still visible
        {-10.0f, 0.0f, -10.0f, 0.0f},
        {10.0f, 0.0f, -10.0f, 0.0f},
        {0.0f, -10.0f, -10.0f, 0.0f},
        {0.0f, 10.0f, -10.0f, 0.0f},
        {-10.0f, -10.0f, -10.0f, 0.0f},
        {10.0f, 10.0f, -10.0f, 0.0f},
        {0.0f, 0.0f, -10.0f, 0.0f},
        {10.0f, -10.0f, -10.0f, 0.0f},
        {-10.0f, 10.0f, -10.0f, 0.0f},
        {0.0f, 0.0f, -10.0f, 0.0f},
        // slightly inside the frustum
        {-1.49f, 0.0f, -0.5f, 0.0f},
        {-10.0f, 0.0f, -100.0f, 0.0f},
        // edge cases where the box is not visible, but is classified as visible
        {-100.51f, 0.0f, -100.0f, 0.0f},
        // box outside frustum planes
        {0.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, -101.0f, 0.0f},
        {-1.51f, 0.0f, -0.5f, 0.0f},
        // edge cases where the box is not visible, with the correct outcome
        {-101.01f, 0.0f, -100.0f, 0.0f}};
    uint32_t test_data_size = 20;

    rpe_compute_t* compute = rpe_compute_init_from_file(driver, "cull.comp.spv", arena);
    TEST_ASSERT_NOT_NULL(compute);

    struct CameraUbo ubo = {0};
    memcpy(ubo.frustums, frustum.planes, sizeof(math_vec4f) * 6);

    struct RenderableExtents extents[20] = {0};
    for (uint32_t i = 0; i < test_data_size; ++i)
    {
        extents[i].center = pass_translations[i];
        extents[i].extent = half_extent;
    }

    buffer_handle_t cam_ubo = rpe_compute_bind_ubo(compute, driver, 0);
    buffer_handle_t scene_ubo = rpe_compute_bind_ubo(compute, driver, 1);

    buffer_handle_t extents_handle =
        rpe_compute_bind_ssbo_host_gpu(compute, driver, 0, test_data_size, 0);
    buffer_handle_t mesh_data_handle =
        rpe_compute_bind_ssbo_host_gpu(compute, driver, 1, test_data_size, 0);
    rpe_compute_bind_ssbo_gpu_host(compute, driver, 2, test_data_size, 0);
    rpe_compute_bind_ssbo_gpu_host(compute, driver, 3, test_data_size, 0);
    buffer_handle_t draw_count_handle = rpe_compute_bind_ssbo_host_gpu(compute, driver, 4, 1, 0);
    buffer_handle_t total_draw_handle = rpe_compute_bind_ssbo_host_gpu(compute, driver, 5, 1, 0);

    uint32_t zero = 0;
    struct IndirectDraw zero_draw[20] = {0};

    vkapi_driver_map_gpu_buffer(driver, cam_ubo, sizeof(rpe_camera_ubo_t), 0, &ubo);
    vkapi_driver_map_gpu_buffer(driver, scene_ubo, sizeof(uint32_t), 0, &test_data_size);
    vkapi_driver_map_gpu_buffer(
        driver, extents_handle, test_data_size * sizeof(struct RenderableExtents), 0, extents);
    vkapi_driver_map_gpu_buffer(
        driver, mesh_data_handle, test_data_size * sizeof(struct IndirectDraw), 0, zero_draw);
    vkapi_driver_map_gpu_buffer(driver, draw_count_handle, sizeof(uint32_t), 0, &zero);
    vkapi_driver_map_gpu_buffer(driver, total_draw_handle, sizeof(uint32_t), 0, &zero);

    vkapi_driver_dispatch_compute(driver, compute->bundle, test_data_size / 128 + 1, 1, 1);

    uint32_t draw_count_host = 0;
    uint32_t total_count_host = 0;
    rpe_compute_download_ssbo_to_host(compute, driver, 4, sizeof(uint32_t), &draw_count_host);
    rpe_compute_download_ssbo_to_host(compute, driver, 5, sizeof(uint32_t), &total_count_host);

    TEST_ASSERT_EQUAL_UINT(16, draw_count_host);
    TEST_ASSERT_EQUAL_UINT(16, total_count_host);

    // These should all pass the visibility check.
    /*for (int i = 0; i < 16; ++i)
    {
        TEST_ASSERT_EQUAL_UINT(1, results[i]);
    }
    // These are outside the frustum so should fail.
    for (int i = 16; i < 20; ++i)
    {
        TEST_ASSERT_EQUAL_UINT(0, results[i]);
    }*/
}