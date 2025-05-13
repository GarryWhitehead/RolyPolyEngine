#include <unity_fixture.h>

// clang-format off
TEST_GROUP_RUNNER(RenderGraphGroup)
{
    RUN_TEST_CASE(RenderGraphGroup, RenderGraph_DepGraph_Tests1)
    RUN_TEST_CASE(RenderGraphGroup, RenderGraph_DepGraph_Tests2)
    RUN_TEST_CASE(RenderGraphGroup, RenderGraph_Tests1)
    RUN_TEST_CASE(RenderGraphGroup, RenderGraph_TestsBasic)
    RUN_TEST_CASE(RenderGraphGroup, RenderGraph_TestsGBuffer)
    RUN_TEST_CASE(RenderGraphGroup, RenderGraph_TestsGBuffer_PresentPass)
}

TEST_GROUP_RUNNER(CommandsGroup)
{
    RUN_TEST_CASE(CommandsGroup, BasicCommands_Test)
}

TEST_GROUP_RUNNER(VisibilityGroup)
{
    RUN_TEST_CASE(VisibilityGroup, AABBox_Test)
    RUN_TEST_CASE(VisibilityGroup, VisCompute_Test)
}

TEST_GROUP_RUNNER(ComputeGroup)
{
    RUN_TEST_CASE(ComputeGroup, TestComputePipeline)
}

TEST_GROUP_RUNNER(EngineGroup)
{
    RUN_TEST_CASE(EngineGroup, General_Test)
}

static void run_all_tests(void)
{
    RUN_TEST_GROUP(RenderGraphGroup)
    RUN_TEST_GROUP(CommandsGroup)
#if RPE_BUILD_GPU_TESTS
    RUN_TEST_GROUP(VisibilityGroup)
    RUN_TEST_GROUP(ComputeGroup)
#endif
    RUN_TEST_GROUP(EngineGroup)
}
// clang-format on

int main(int argc, const char* argv[]) { return UnityMain(argc, argv, run_all_tests); }