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

static void run_all_tests(void)
{
    RUN_TEST_GROUP(RenderGraphGroup)
}
// clang-format on

int main(int argc, const char* argv[]) { return UnityMain(argc, argv, run_all_tests); }