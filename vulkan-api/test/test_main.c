#define UNITY_FIXTURE_NO_EXTRAS 1
#include <unity_fixture.h>

// clang-format off
TEST_GROUP_RUNNER(ShaderUtilGroup)
{
    RUN_TEST_CASE(ShaderUtilGroup, ShaderUtil_ParseDefinition)
    RUN_TEST_CASE(ShaderUtilGroup, ShaderUtil_PreprocessShader_Multi)
    RUN_TEST_CASE(ShaderUtilGroup, ShaderUtil_PreprocessShader_Single)
    RUN_TEST_CASE(ShaderUtilGroup, ShaderUtil_IncludeAppend)
}

TEST_GROUP_RUNNER(ProgramManagerGroup)
{
    RUN_TEST_CASE(ProgramManagerGroup, PM_ShaderProgram_Tests)
}

TEST_GROUP_RUNNER(ShaderGroup)
{
    RUN_TEST_CASE(ShaderGroup, Shader_CompilerTests)
}

static void run_all_tests()
{
    RUN_TEST_GROUP(ShaderUtilGroup)
    RUN_TEST_GROUP(ProgramManagerGroup)
    RUN_TEST_GROUP(ShaderGroup)
}

// clang-format on

int main(int argc, const char* argv[]) { return UnityMain(argc, argv, run_all_tests); }