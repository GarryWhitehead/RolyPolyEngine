
#include <unity_fixture.h>

#include <vulkan-api/shader.h>
#include <vulkan-api/context.h>
#include <vulkan-api/driver.h>
#include <vulkan-api/error_codes.h>
#include <backend/enums.h>

#include <string.h>

TEST_GROUP(ShaderGroup);

TEST_SETUP(ShaderGroup) {}

TEST_TEAR_DOWN(ShaderGroup) {}

TEST(ShaderGroup, Shader_CompilerTests)
{
    arena_t arena;
    uint64_t arena_cap = 1 << 15;
    int res = arena_new(arena_cap, &arena);
    TEST_ASSERT(ARENA_SUCCESS == res);

    const char* test_shader0 = "#version 460\n"
                              "\n"
                              "void main()\n"
                              "{\n"
                              "float val1 = 0;\n"
                              "float val2 = 3;\n"
                              "float val3 = val1 + val2;\n"
                              "}\n";

    vkapi_driver_t driver;
    int error_code = vkapi_driver_init(NULL, 0, &driver);
    TEST_ASSERT(error_code == VKAPI_SUCCESS);
    error_code = vkapi_driver_create_device(&driver, NULL);
    TEST_ASSERT(error_code == VKAPI_SUCCESS);

    shader_t* shader = shader_init(RPE_BACKEND_SHADER_STAGE_VERTEX, &arena);

    bool r = shader_compile(shader, &driver.context, test_shader0, "test_path", &arena);
    TEST_ASSERT(r == true);

    // Test shader reflection works.
    const char* test_shader1 = "#version 460\n"
                              "\n"
                               "layout(location = 0) in vec3 inPos;\n"
                               "layout(location = 1) in vec2 inUv;\n"
                               "layout(location = 0) out vec2 outUv;\n"
                               "layout(location = 1) out vec3 outNormal;\n"
                               "layout(constant_id = 0) const int LightTypePoint = 0;\n"
                               "\n"
                               "layout(binding = 0) uniform Buffer {\n"
                               "    mat4 m;\n"
                               "    mat4 v;\n"
                               "    mat4 p;\n"
                               "} ubo;\n"
                               "layout(binding = 1, set = 3) uniform sampler2D texSampler;\n"
                              "void main()\n"
                              "{\n"
                              "float val1 = 0;\n"
                              "float val2 = 3;\n"
                              "float val3 = val1 + val2;\n"
                               "outUv = inUv;"
                              "}\n";

    r = shader_compile(shader, &driver.context, test_shader1, "test_path", &arena);
    TEST_ASSERT(r == true);

    shader_binding_t* resource_binding = shader_get_resource_binding(shader);
    TEST_ASSERT_EQUAL_UINT(2, resource_binding->stage_output_count);
    TEST_ASSERT_EQUAL_UINT(2, resource_binding->stage_input_count);

    // Input attributes.
    // Note: The shader reflection library places the bindings in an unsorted order.
    TEST_ASSERT_EQUAL_UINT(1, resource_binding->stage_inputs[0].location);
    TEST_ASSERT_EQUAL_UINT(8, resource_binding->stage_inputs[0].stride);
    TEST_ASSERT(resource_binding->stage_inputs[0].format == VK_FORMAT_R32G32_SFLOAT);

    TEST_ASSERT_EQUAL_UINT(0, resource_binding->stage_inputs[1].location);
    TEST_ASSERT_EQUAL_UINT(12, resource_binding->stage_inputs[1].stride);
    TEST_ASSERT(resource_binding->stage_inputs[1].format == VK_FORMAT_R32G32B32_SFLOAT);

    // Output attributes.
    TEST_ASSERT_EQUAL_UINT(0, resource_binding->stage_outputs[0].location);
    TEST_ASSERT_EQUAL_UINT(8, resource_binding->stage_outputs[0].stride);
    TEST_ASSERT(resource_binding->stage_outputs[0].format == VK_FORMAT_R32G32_SFLOAT);

    TEST_ASSERT_EQUAL_UINT(1, resource_binding->stage_outputs[1].location);
    TEST_ASSERT_EQUAL_UINT(12, resource_binding->stage_outputs[1].stride);
    TEST_ASSERT(resource_binding->stage_outputs[1].format == VK_FORMAT_R32G32B32_SFLOAT);

    // Ubos, samplers, etc.
    TEST_ASSERT_EQUAL_UINT(2, resource_binding->desc_layout_count);
    // Sampler.
    TEST_ASSERT_EQUAL_UINT(1, resource_binding->desc_layouts[0].binding);
    TEST_ASSERT_EQUAL_UINT(3, resource_binding->desc_layouts[0].set);
    TEST_ASSERT(resource_binding->desc_layouts[0].stage == VK_SHADER_STAGE_VERTEX_BIT);
    TEST_ASSERT(resource_binding->desc_layouts[0].type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    // UBO.
    TEST_ASSERT_EQUAL_UINT(0, resource_binding->desc_layouts[1].binding);
    TEST_ASSERT_EQUAL_UINT(0, resource_binding->desc_layouts[1].set);
    TEST_ASSERT(resource_binding->desc_layouts[1].stage == VK_SHADER_STAGE_VERTEX_BIT);
    TEST_ASSERT(resource_binding->desc_layouts[1].type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    TEST_ASSERT_EQUAL_UINT(64 * 3, resource_binding->desc_layouts[1].range);

    vkapi_driver_shutdown(&driver);
}
