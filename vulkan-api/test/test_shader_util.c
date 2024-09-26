#include <string.h>
#include <unity_fixture.h>
#include <utility/arena.h>
#include <utility/string.h>
#include <vulkan-api/program_manager.h>
#include <vulkan-api/shader_util.h>

TEST_GROUP(ShaderUtilGroup);

TEST_SETUP(ShaderUtilGroup) {}

TEST_TEAR_DOWN(ShaderUtilGroup) {}

TEST(ShaderUtilGroup, ShaderUtil_ParseDefinition)
{
    arena_t arena;
    uint64_t arena_cap = 1 << 25;
    int res = arena_new(arena_cap, &arena);
    TEST_ASSERT(ARENA_SUCCESS == res);

    string_t test_line1 = string_init(
        "#if (defined(TEST_DEF1) && defined(TEST_DEF2)) || (defined(TEST_DEF3) && "
        "defined(TEST_DEF4))",
        &arena);
    variant_t variants_test1[4];
    variants_test1[0].definition = string_init("TEST_DEF1", &arena);
    variants_test1[1].definition = string_init("TEST_DEF2", &arena);
    variants_test1[2].definition = string_init("TEST_DEF3", &arena);
    variants_test1[3].definition = string_init("TEST_DEF4", &arena);

    // Test all definitions present - all definition passed - should be true.
    bool error;
    bool parse_res = pp_parse_if(&test_line1, variants_test1, 4, &arena, 0, &error);
    TEST_ASSERT(error == false);
    TEST_ASSERT(parse_res == true);

    // Test with only TEST_DEP1 and TEST_DEP2 variants passed, should still pass.
    parse_res = pp_parse_if(&test_line1, variants_test1, 2, &arena, 0, &error);
    TEST_ASSERT(error == false);
    TEST_ASSERT(parse_res == true);

    // Test with no TEST_DEF1 stated, should pass
    variants_test1[0].definition = string_init("TEST_INVALID1", &arena);
    parse_res = pp_parse_if(&test_line1, variants_test1, 4, &arena, 0, &error);
    TEST_ASSERT(error == false);
    TEST_ASSERT(parse_res == true);

    // Test with TEST_DEF1 and TEST_DEF4, should fail now.
    variants_test1[3].definition = string_init("TEST_INVALID4", &arena);
    parse_res = pp_parse_if(&test_line1, variants_test1, 4, &arena, 0, &error);
    TEST_ASSERT(error == false);
    TEST_ASSERT(parse_res == false);

    variants_test1[0].definition = string_init("TEST_DEF1", &arena);
    variants_test1[3].definition = string_init("TEST_DEF4", &arena);

    string_t test_line2 = string_init(
        "#if (defined(TEST_DEF1) || defined(TEST_DEF2)) && (defined(TEST_DEF3) || "
        "defined(TEST_DEF4))",
        &arena);
    parse_res = pp_parse_if(&test_line2, variants_test1, 4, &arena, 0, &error);
    TEST_ASSERT(error == false);
    TEST_ASSERT(parse_res == true);

    // Test with only TEST_DEP1 and TEST_DEP2 variants passed, should fail.
    parse_res = pp_parse_if(&test_line2, variants_test1, 2, &arena, 0, &error);
    TEST_ASSERT(error == false);
    TEST_ASSERT(parse_res == false);

    // Test with all variants except TEST_DEP1, should pass.
    variants_test1[0].definition = string_init("TEST_INVALID1", &arena);
    parse_res = pp_parse_if(&test_line2, variants_test1, 4, &arena, 0, &error);
    TEST_ASSERT(error == false);
    TEST_ASSERT(parse_res == true);

    // Test with TEST_DEP1 and TEST_DEP4 not stated, should pass still.
    variants_test1[3].definition = string_init("TEST_INVALID4", &arena);
    parse_res = pp_parse_if(&test_line2, variants_test1, 4, &arena, 0, &error);
    TEST_ASSERT(error == false);
    TEST_ASSERT(parse_res == true);

    string_t test_line3 = string_init(
        "#if defined(TEST_DEF1) || defined(TEST_DEF2) || defined(TEST_DEF3) || defined(TEST_DEF4)",
        &arena);
    parse_res = pp_parse_if(&test_line3, variants_test1, 4, &arena, 0, &error);
    TEST_ASSERT(error == false);
    TEST_ASSERT(parse_res == true);

    // TEST_DEP1 and TEST_DEP4 not defined so should fail.
    string_t test_line4 = string_init(
        "#if defined(TEST_DEF1) && defined(TEST_DEF2) && defined(TEST_DEF3) && defined(TEST_DEF4)",
        &arena);
    parse_res = pp_parse_if(&test_line4, variants_test1, 4, &arena, 0, &error);
    TEST_ASSERT(error == false);
    TEST_ASSERT(parse_res == false);

    // But this should pass.
    string_t test_line5 = string_init(
        "#if !defined(TEST_DEF1) && defined(TEST_DEF2) && defined(TEST_DEF3) && "
        "!defined(TEST_DEF4)",
        &arena);
    parse_res = pp_parse_if(&test_line5, variants_test1, 4, &arena, 0, &error);
    TEST_ASSERT(error == false);
    TEST_ASSERT(parse_res == true);

    // And this should fail.
    string_t test_line6 = string_init(
        "#if defined(TEST_DEF1) || !defined(TEST_DEF2) || !defined(TEST_DEF3) || "
        "defined(TEST_DEF4)",
        &arena);
    parse_res = pp_parse_if(&test_line6, variants_test1, 4, &arena, 0, &error);
    TEST_ASSERT(error == false);
    TEST_ASSERT(parse_res == false);

    // Single definition checks.
    string_t test_line7 = string_init("#if defined(TEST_DEF1)", &arena);
    parse_res = pp_parse_if(&test_line7, variants_test1, 4, &arena, 0, &error);
    TEST_ASSERT(error == false);
    TEST_ASSERT(parse_res == false);

    string_t test_line8 = string_init("#if !defined(TEST_DEF1)", &arena);
    parse_res = pp_parse_if(&test_line8, variants_test1, 4, &arena, 0, &error);
    TEST_ASSERT(error == false);
    TEST_ASSERT(parse_res == true);
}

TEST(ShaderUtilGroup, ShaderUtil_PreprocessShader_Multi)
{
    arena_t arena;
    uint64_t arena_cap = 1 << 25;
    int res = arena_new(arena_cap, &arena);
    TEST_ASSERT(ARENA_SUCCESS == res);

    arena_t scratch_arena;
    uint64_t scratch_arena_cap = 1 << 25;
    res = arena_new(scratch_arena_cap, &scratch_arena);
    TEST_ASSERT(ARENA_SUCCESS == res);

    const char* test_block = "\n"
                             "#if defined(TEST_DEF1) && !defined(TEST_DEF2)\n"
                             "int var = 0;\n"
                             "int var1 = 1;\n"
                             "#elif defined(TEST_DEF3) || defined(TEST_DEF4)\n"
                             "int var = 1;\n"
                             "int var1 = 2;\n"
                             "#elif !defined(TEST_DEF5) && defined(TEST_DEF6)\n"
                             "int var = 3;\n"
                             "int var1 = 5;\n"
                             "#else\n"
                             "int var = 100;\n"
                             "int var1 = 200;\n"
                             "#endif";

    variant_t variants_test1[2];
    variants_test1[0].definition = string_init("TEST_DEF1", &arena);
    variants_test1[1].definition = string_init("TEST_DEF2", &arena);

    string_t test_str = string_init(test_block, &scratch_arena);
    string_t shader_block =
        shader_program_process_preprocessor(&test_str, variants_test1, 2, &arena, &scratch_arena);
    TEST_ASSERT_NOT_NULL(shader_block.data);
    TEST_ASSERT_EQUAL_STRING("\nint var = 0;\nint var1 = 1;\n", shader_block.data);
    TEST_ASSERT_EQUAL_size_t(28, shader_block.len);

    variants_test1[0].definition = string_init("TEST_DEF3", &arena);
    variants_test1[1].definition = string_init("TEST_DEF4", &arena);

    test_str = string_init(test_block, &scratch_arena);
    shader_block =
        shader_program_process_preprocessor(&test_str, variants_test1, 2, &arena, &scratch_arena);
    TEST_ASSERT_NOT_NULL(shader_block.data);
    TEST_ASSERT_EQUAL_STRING("\nint var = 1;\nint var1 = 2;\n", shader_block.data);
    TEST_ASSERT_EQUAL_size_t(28, shader_block.len);

    // Should still pass if only 'TEST_DEF3' is passed as a variant.
    test_str = string_init(test_block, &scratch_arena);
    shader_block =
        shader_program_process_preprocessor(&test_str, variants_test1, 1, &arena, &scratch_arena);
    TEST_ASSERT_NOT_NULL(shader_block.data);
    TEST_ASSERT_EQUAL_STRING("\nint var = 1;\nint var1 = 2;\n", shader_block.data);
    TEST_ASSERT_EQUAL_size_t(28, shader_block.len);

    test_str = string_init(test_block, &scratch_arena);
    shader_block =
        shader_program_process_preprocessor(&test_str, variants_test1, 0, &arena, &scratch_arena);
    TEST_ASSERT_NOT_NULL(shader_block.data);
    TEST_ASSERT_EQUAL_STRING("\nint var = 100;\nint var1 = 200;\n", shader_block.data);
    TEST_ASSERT_EQUAL_size_t(32, shader_block.len);
}

TEST(ShaderUtilGroup, ShaderUtil_PreprocessShader_Single)
{
    arena_t arena;
    uint64_t arena_cap = 1 << 20;
    int res = arena_new(arena_cap, &arena);
    TEST_ASSERT(ARENA_SUCCESS == res);

    arena_t scratch_arena;
    uint64_t scratch_arena_cap = 1 << 20;
    res = arena_new(scratch_arena_cap, &scratch_arena);
    TEST_ASSERT(ARENA_SUCCESS == res);

    // Test on some "real-ife" code.
    const char* test_block = "#version 410\n"
                             "#if defined(HAS_UV_ATTR_INPUT)\n"
                             "layout(location = 0) in vec2 inUv;\n"
                             "#endif\n"
                             "#if defined(HAS_NORMAL_ATTR_INPUT)\n"
                             "layout(location = 1) in vec3 inNormal;\n"
                             "#endif\n"
                             "void main()\n"
                             "{\n"
                             "    // albedo\n"
                             "    vec4 baseColour = vec4(1.0);\n"
                             "    float alphaMask = 1.0;\n"
                             "\n"
                             "#if defined(HAS_ALPHA_MASK)\n"
                             "    alphaMask = material_ubo.alphaMask;\n"
                             "#endif\n"
                             "#if defined(HAS_NORMAL_SAMPLER) && defined(HAS_UV_ATTR_INPUT)\n"
                             "    normal = peturbNormal(inUv);\n"
                             "#elif defined(HAS_NORMAL_ATTR_INPUT)\n"
                             "    normal = normalize(inNormal);\n"
                             "#else\n"
                             "    normal = normalize(cross(dFdx(inPos), dFdy(inPos)));\n"
                             "#endif\n"
                             "}";

    const char* expected1 = "#version 410\n"
                            "layout(location = 0) in vec2 inUv;\n"
                            "void main()\n"
                            "{\n"
                            "    // albedo\n"
                            "    vec4 baseColour = vec4(1.0);\n"
                            "    float alphaMask = 1.0;\n"
                            "\n"
                            "    normal = peturbNormal(inUv);\n"
                            "}";

    const char* expected2 = "#version 410\n"
                            "layout(location = 1) in vec3 inNormal;\n"
                            "void main()\n"
                            "{\n"
                            "    // albedo\n"
                            "    vec4 baseColour = vec4(1.0);\n"
                            "    float alphaMask = 1.0;\n"
                            "\n"
                            "    alphaMask = material_ubo.alphaMask;\n"
                            "    normal = normalize(inNormal);\n"
                            "}";

    const char* expected3 = "#version 410\n"
                            "void main()\n"
                            "{\n"
                            "    // albedo\n"
                            "    vec4 baseColour = vec4(1.0);\n"
                            "    float alphaMask = 1.0;\n"
                            "\n"
                            "    normal = normalize(cross(dFdx(inPos), dFdy(inPos)));\n"
                            "}";

    variant_t variants_test1[2];
    variants_test1[0].definition = string_init("HAS_UV_ATTR_INPUT", &arena);
    variants_test1[1].definition = string_init("HAS_NORMAL_SAMPLER", &arena);

    string_t test_str = string_init(test_block, &scratch_arena);
    string_t shader_block =
        shader_program_process_preprocessor(&test_str, variants_test1, 2, &arena, &scratch_arena);
    TEST_ASSERT_NOT_NULL(shader_block.data);
    TEST_ASSERT_EQUAL_STRING(expected1, shader_block.data);

    variants_test1[0].definition = string_init("HAS_NORMAL_ATTR_INPUT", &arena);
    variants_test1[1].definition = string_init("HAS_ALPHA_MASK", &arena);

    test_str = string_init(test_block, &scratch_arena);
    shader_block =
        shader_program_process_preprocessor(&test_str, variants_test1, 2, &arena, &scratch_arena);
    TEST_ASSERT_NOT_NULL(shader_block.data);
    TEST_ASSERT_EQUAL_STRING(expected2, shader_block.data);

    test_str = string_init(test_block, &scratch_arena);
    shader_block =
        shader_program_process_preprocessor(&test_str, variants_test1, 0, &arena, &scratch_arena);
    TEST_ASSERT_NOT_NULL(shader_block.data);
    TEST_ASSERT_EQUAL_STRING(expected3, shader_block.data);
}

TEST(ShaderUtilGroup, ShaderUtil_IncludeAppend)
{
    const char* shader_block = "#version 410\n"
                               "layout(location = 0) in vec2 inUv;\n"
                               "/n";

    const char* expected = "#version 410\n"
                           "layout(location = 0) in vec2 inUv;\n"
                           "/n"
                           "#ifndef MATH_H\n"
                           "#define MATH_H\n"
                           "\n"
                           "#define PI 3.14159265359\n"
                           "#define HALF_PI 1.570796327\n"
                           "\n"
                           "#define GRAVITY 9.81\n"
                           "\n"
                           "#endif";

    arena_t arena;
    uint64_t arena_cap = 1 << 20;
    int res = arena_new(arena_cap, &arena);
    TEST_ASSERT(ARENA_SUCCESS == res);

    string_t block_str = string_init(shader_block, &arena);
    string_t inc_path = string_init("include/math.h", &arena);
    bool r = shader_util_append_include_file(&block_str, &inc_path, &arena);
    TEST_ASSERT(r == true);
    TEST_ASSERT_EQUAL_STRING(expected, block_str.data);
}
