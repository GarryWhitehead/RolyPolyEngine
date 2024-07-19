#include <unity_fixture.h>

TEST_GROUP_RUNNER(ArrayGroup) {RUN_TEST_CASE(ArrayGroup, GenericTests)}

TEST_GROUP_RUNNER(VectorGroup) {RUN_TEST_CASE(VectorGroup, GenericTests)}

TEST_GROUP_RUNNER(ArenaGroup) {RUN_TEST_CASE(ArenaGroup, ArenaTests_GeneralTests)
                                   RUN_TEST_CASE(ArenaGroup, ArenaTests_DynamicArray)
                                       RUN_TEST_CASE(ArenaGroup, ArenaTests_DynamicArrayWithChar)}

TEST_GROUP_RUNNER(HashSetGroup)
{
    RUN_TEST_CASE(HashSetGroup, HashSet_GeneralTests);
    RUN_TEST_CASE(HashSetGroup, HashSet_ResizeTests)
}

TEST_GROUP_RUNNER(JobQueueGroup) {RUN_TEST_CASE(JobQueueGroup, JobQueue_GeneralTests)
                                      RUN_TEST_CASE(JobQueueGroup, JobQueue_JobWithChildrenTests)}

TEST_GROUP_RUNNER(WorkStealingQueueGroup)
{
    RUN_TEST_CASE(WorkStealingQueueGroup, WorkStealingQueue_GeneralTests)
}

static void run_all_tests()
{
    RUN_TEST_GROUP(ArrayGroup);
    RUN_TEST_GROUP(ArenaGroup);
    RUN_TEST_GROUP(VectorGroup);
    RUN_TEST_GROUP(HashSetGroup);
    RUN_TEST_GROUP(JobQueueGroup)
    RUN_TEST_GROUP(WorkStealingQueueGroup)
}

int main(int argc, const char* argv[]) { return UnityMain(argc, argv, run_all_tests); }