#include <unity_fixture.h>

TEST_GROUP_RUNNER(ArrayGroup) {RUN_TEST_CASE(ArrayGroup, GenericTests)}

TEST_GROUP_RUNNER(VectorGroup) {RUN_TEST_CASE(VectorGroup, GenericTests)}

TEST_GROUP_RUNNER(ArenaGroup)
{
    RUN_TEST_CASE(ArenaGroup, GenericTests)
    RUN_TEST_CASE(ArenaGroup, ArenaDynamicArray)
}

static void run_all_tests()
{
    RUN_TEST_GROUP(ArrayGroup);
    RUN_TEST_GROUP(ArenaGroup);
    RUN_TEST_GROUP(VectorGroup);
}

int main(int argc, const char* argv[]) { return UnityMain(argc, argv, run_all_tests); }