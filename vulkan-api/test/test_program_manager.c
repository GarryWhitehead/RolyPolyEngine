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

    vkapi_driver_t driver;
    int error_code = vkapi_driver_init(NULL, 0, &driver);
    TEST_ASSERT(error_code == VKAPI_SUCCESS);
    error_code = vkapi_driver_create_device(&driver, NULL);
    TEST_ASSERT(error_code == VKAPI_SUCCESS);

    program_manager_t* m = program_manager_init(&arena);
    shader_prog_bundle_t* bundle = shader_bundle_init(&arena);

    const char* shader_vert_filename = "post_process.vert";
    const char* shader_frag_filename = "post_process.frag";
    const char* mat_filename = "bloom.glsl";

    bool r = shader_bundle_build_shader(
        bundle,
        shader_vert_filename,
        RPE_BACKEND_SHADER_STAGE_VERTEX,
        NULL,
        0,
        &arena,
        &scratch_arena);
    TEST_ASSERT(r);

    r = shader_bundle_build_shader(
        bundle,
        shader_frag_filename,
        RPE_BACKEND_SHADER_STAGE_FRAGMENT,
        NULL,
        0,
        &arena,
        &scratch_arena);
    TEST_ASSERT(r);

    r = shader_bundle_parse_mat_shader(bundle, mat_filename, &arena, &scratch_arena);
    TEST_ASSERT(r);

    // The attributes are normally created outside of the vulkan api, but for testing purposes, doing it manually here.
    string_t attr_block0 = string_init("layout(binding = 1, set = 3) uniform sampler2D ColourSampler;\n", &arena);
    string_t attr_block1 = string_init("layout(binding = 2, set = 3) uniform sampler2D LuminanceAvgLut;\n", &arena);
    string_t attr_block2 = string_init("layout(binding = 0) uniform Buffer\n{\n   float gamma;\n} material_ubo;\n", &arena);
    shader_program_t* prog = shader_bundle_get_stage_program(bundle, RPE_BACKEND_SHADER_STAGE_FRAGMENT);
    TEST_ASSERT(prog);
    shader_program_add_attr_block(prog, &attr_block0);
    shader_program_add_attr_block(prog, &attr_block1);
    shader_program_add_attr_block(prog, &attr_block2);

    r = program_manager_find_shader_variant_or_create(
        m,
        &driver.context,
        RPE_BACKEND_SHADER_STAGE_VERTEX,
        VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
        bundle,
        0,
        &arena);
    TEST_ASSERT(r);

    r = program_manager_find_shader_variant_or_create(
        m,
        &driver.context,
        RPE_BACKEND_SHADER_STAGE_FRAGMENT,
        VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
        bundle,
        0,
        &arena);
    TEST_ASSERT(r);
}