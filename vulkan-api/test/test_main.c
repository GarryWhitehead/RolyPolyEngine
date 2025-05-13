#define UNITY_FIXTURE_NO_EXTRAS 1
#include <unity_fixture.h>

// clang-format off
TEST_GROUP_RUNNER(ProgramManagerGroup)
{
    RUN_TEST_CASE(ProgramManagerGroup, PM_ShaderProgram_Tests)
}

TEST_GROUP_RUNNER(ShaderGroup)
{
    RUN_TEST_CASE(ShaderGroup, Shader_CompilerTests)
}

TEST_GROUP_RUNNER(CacheGroup)
{
    RUN_TEST_CASE(CacheGroup, KeyCompare_Test)
}

static void run_all_tests()
{
    RUN_TEST_GROUP(ProgramManagerGroup)
    RUN_TEST_GROUP(ShaderGroup)
    RUN_TEST_GROUP(CacheGroup)
}

// clang-format on

int main(int argc, const char* argv[]) { return UnityMain(argc, argv, run_all_tests); }
