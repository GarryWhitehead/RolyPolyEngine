#include <unity_fixture.h>

// clang-format off
TEST_GROUP_RUNNER(ArrayGroup)
{
    RUN_TEST_CASE(ArrayGroup, GenericTests)
}

TEST_GROUP_RUNNER(VectorGroup)
{
    RUN_TEST_CASE(VectorGroup, GenericTests)
}

TEST_GROUP_RUNNER(ArenaGroup)
{
    RUN_TEST_CASE(ArenaGroup, ArenaTests_GeneralTests)
    RUN_TEST_CASE(ArenaGroup, ArenaTests_DynamicArray)
    RUN_TEST_CASE(ArenaGroup, ArenaTests_DynamicArrayWithChar)
}

TEST_GROUP_RUNNER(HashSetGroup)
{
    RUN_TEST_CASE(HashSetGroup, HashSet_GeneralTests);
    RUN_TEST_CASE(HashSetGroup, HashSet_ResizeTests)
}

TEST_GROUP_RUNNER(JobQueueGroup)
{
    RUN_TEST_CASE(JobQueueGroup, JobQueue_GeneralTests)
    RUN_TEST_CASE(JobQueueGroup, JobQueue_JobWithChildrenTests)
}

TEST_GROUP_RUNNER(WorkStealingQueueGroup)
{
    RUN_TEST_CASE(WorkStealingQueueGroup, WorkStealingQueue_GeneralTests)
}

TEST_GROUP_RUNNER(MathGroup)
{
    RUN_TEST_CASE(MathGroup, MathTests_Vector2)
    RUN_TEST_CASE(MathGroup, MathTests_Vector3)
    RUN_TEST_CASE(MathGroup, MathTests_Vector4)
    RUN_TEST_CASE(MathGroup, MathTests_Mat3)
    RUN_TEST_CASE(MathGroup, MathTests_Mat4)
}

TEST_GROUP_RUNNER(StringGroup)
{
    RUN_TEST_CASE(StringGroup, StringTests_General)
}

TEST_GROUP_RUNNER(FilesystemGroup)
{
    RUN_TEST_CASE(FilesystemGroup, Filesystem_Extension)
}

static void run_all_tests()
{
    RUN_TEST_GROUP(ArrayGroup)
    RUN_TEST_GROUP(ArenaGroup)
    RUN_TEST_GROUP(VectorGroup)
    RUN_TEST_GROUP(HashSetGroup)
    RUN_TEST_GROUP(JobQueueGroup)
    RUN_TEST_GROUP(WorkStealingQueueGroup)
    RUN_TEST_GROUP(MathGroup)
    RUN_TEST_GROUP(StringGroup)
    RUN_TEST_GROUP(FilesystemGroup)
}
// clang-format on

int main(int argc, const char* argv[]) { return UnityMain(argc, argv, run_all_tests); }