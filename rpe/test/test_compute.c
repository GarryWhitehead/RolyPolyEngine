#include "vk_setup.h"

#include <compute.h>
#include <unity_fixture.h>

#include <string.h>

TEST_GROUP(ComputeGroup);

TEST_SETUP(ComputeGroup) {}

TEST_TEAR_DOWN(ComputeGroup) {}

TEST(ComputeGroup, TestComputePipeline)
{
    const char* test_shader = "#version 460\n"
                              "layout (set = 2, binding = 0) readonly buffer InSsbo\n"
                              "{\n"
                              "\tint data[];\n"
                              "} input_ssbo;\n"
                              "layout (set = 2, binding = 1) buffer OutSsbo\n"
                              "{\n"
                              "\tint data[];\n"
                              "} output_ssbo;\n"
                              "layout (set = 0, binding = 0) uniform ComputeUbo\n"
                              "{\n"
                              "\tint N;\n"
                              "} compute_ubo;\n"
                              "\n"
                              "layout (local_Size_x = 16, local_Size_y = 1) in;\n"
                              "\n"
                              "void main()\n"
                              "{\n"
                              "uint idx = gl_GlobalInvocationID.x;\n"
                              "if (idx > compute_ubo.N)\n"
                              "{\n"
                              "\treturn;\n"
                              "}\n"
                              "\toutput_ssbo.data[idx] = input_ssbo.data[idx];\n"
                              "}\n";

    arena_t* arena = setup_arena(1 << 20);
    vkapi_driver_t* driver = setup_driver();

    rpe_compute_t* compute = rpe_compute_init_from_text(driver, test_shader, arena);
    TEST_ASSERT_NOT_NULL(compute);

    int data_size = 1000;
    int host_data[1000] = {0};
    int in_data[1000];
    for (int i = 0; i < 1000; ++i)
    {
        in_data[i] = i * 2;
    }
    RENDERDOC_START_CAPTURE(NULL, NULL)

    buffer_handle_t in_ssbo_handle =
        rpe_compute_bind_ssbo_host_gpu(compute, driver, 0, data_size, 0);
    rpe_compute_bind_ssbo_gpu_host(compute, driver, 1, data_size, 0);
    buffer_handle_t ubo_handle = rpe_compute_bind_ubo(compute, driver, 0);

    vkapi_driver_map_gpu_buffer(driver, in_ssbo_handle, data_size * sizeof(int), 0, in_data);
    vkapi_driver_map_gpu_buffer(driver, ubo_handle, sizeof(int), 0, &data_size);

    vkapi_driver_dispatch_compute(driver, compute->bundle, data_size / 16 + 1, 1, 1);

    rpe_compute_download_ssbo_to_host(compute, driver, 1, data_size * sizeof(int), host_data);

    RENDERDOC_END_CAPTURE(NULL, NULL)
    TEST_ASSERT(memcmp(host_data, in_data, data_size * sizeof(int)) == 0);
}