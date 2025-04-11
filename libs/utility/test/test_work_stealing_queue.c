#include "unity.h"
#include "unity_fixture.h"
#include "utility/arena.h"
#include "utility/work_stealing_queue.h"

TEST_GROUP(WorkStealingQueueGroup);

TEST_SETUP(WorkStealingQueueGroup) {}

TEST_TEAR_DOWN(WorkStealingQueueGroup) {}

TEST(WorkStealingQueueGroup, WorkStealingQueue_GeneralTests)
{
    arena_t arena;
    uint64_t arena_cap = 1 << 15;
    int res = arena_new(arena_cap, &arena);
    TEST_ASSERT(ARENA_SUCCESS == res);

    int work_size = 1024;
    work_stealing_queue_t queue = work_stealing_queue_init(&arena, work_size);

    // push/pop
    for (int i = 0; i < work_size; ++i)
    {
        work_stealing_queue_push(&queue, i);
    }
    for (int i = 0; i < work_size; ++i)
    {
        res = work_stealing_queue_pop(&queue);
        TEST_ASSERT_EQUAL_INT((work_size - 1) - i, res);
    }

    // push/steal
    for (int i = 0; i < work_size; ++i)
    {
        work_stealing_queue_push(&queue, i);
    }
    for (int i = 0; i < work_size; ++i)
    {
        res = work_stealing_queue_steal(&queue);
        TEST_ASSERT_EQUAL_INT(i, res);
    }
}