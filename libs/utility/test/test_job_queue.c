#include "unity.h"
#include "unity_fixture.h"
#include "utility/arena.h"
#include "utility/job_queue.h"

#include "utility/thread.h"

TEST_GROUP(JobQueueGroup);

TEST_SETUP(JobQueueGroup) {}

TEST_TEAR_DOWN(JobQueueGroup) {}

struct ThreadParams1
{
    int* array;
    int index;
};

void thread_func1(void* arg)
{
    struct ThreadParams1* params = arg;
    params->array[params->index] *= 5;
}

TEST(JobQueueGroup, JobQueue_GeneralTests)
{
    arena_t arena;
    uint64_t arena_cap = 1 << 18;
    int res = arena_new(arena_cap, &arena);
    TEST_ASSERT(ARENA_SUCCESS == res);

    int thread_count = 2;
    job_queue_t* jq = job_queue_init(&arena, thread_count, 1);
    job_queue_adopt_thread(jq);

    int arr[] = {2, 6, 10};
    int arr_size = 6;
    job_t** jobs = malloc(arr_size * sizeof(job_t));
    struct ThreadParams1* params = malloc(arr_size * sizeof(struct ThreadParams1));

    for (int i = 0; i < arr_size; ++i)
    {
        struct ThreadParams1* param = &params[i];
        param->array = arr;
        param->index = i;
        jobs[i] = job_queue_create_job(jq, &thread_func1, param, NULL);
    }

    job_queue_run_and_wait(jq, jobs[0]);
    job_queue_run_and_wait(jq, jobs[1]);
    job_queue_run_and_wait(jq, jobs[2]);

    TEST_ASSERT_EQUAL_INT(10, arr[0]);
    TEST_ASSERT_EQUAL_INT(30, arr[1]);
    TEST_ASSERT_EQUAL_INT(50, arr[2]);

    job_queue_destroy(jq);
    free(params);
    free(jobs);
}

atomic_int counter;
void thread_func2(void* arg) { counter++; }

TEST(JobQueueGroup, JobQueue_JobWithChildrenTests)
{
    arena_t arena;
    uint64_t arena_cap = 1 << 18;
    int res = arena_new(arena_cap, &arena);
    TEST_ASSERT(ARENA_SUCCESS == res);

    int thread_count = 3;
    job_queue_t* jq = job_queue_init(&arena, thread_count, 1);
    job_queue_adopt_thread(jq);

    int work_size = 20;
    job_t** jobs = malloc(work_size * sizeof(job_t));
    counter = 0;
    jobs[0] = job_queue_create_job(jq, &thread_func2, NULL, NULL);

    for (int i = 1; i < work_size; ++i)
    {
        jobs[i] = job_queue_create_job(jq, &thread_func2, NULL, jobs[0]);
        job_queue_run_job(jq, jobs[i]);
    }
    job_queue_run_and_wait(jq, jobs[0]);

    TEST_ASSERT_EQUAL_INT(work_size, counter);

    job_queue_destroy(jq);
    free(jobs);
}