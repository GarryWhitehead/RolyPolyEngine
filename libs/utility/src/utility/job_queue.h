/* Copyright (c) 2024 Garry Whitehead
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef __UTILITY_JOB_QUEUE_H__
#define __UTILITY_JOB_QUEUE_H__

#define _GNU_SOURCE
#include "compiler.h"
#include "hash_set.h"
#include "random.h"
#include "thread.h"
#include "work_stealing_queue.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#define JOB_QUEUE_MAX_JOB_COUNT 1024
#define JOB_QUEUE_MAX_THREAD_COUNT 32
#define JOB_QUEUE_CACHELINE_SIZE 64

// Forward declarations.
typedef struct JobQueue job_queue_t;

typedef void (*job_func_t)(void*);

/**
 Information on each job created.
 */
typedef struct RPE_ALIGNAS(JOB_QUEUE_CACHELINE_SIZE) Job
{
    /// Function pointer to the function which will be called by this job.
    job_func_t func;
    /// Arguments to pass to the function.
    void* args;
    /// The number of references to this job.
    atomic_int_fast16_t ref_count;
    /// The number of jobs running for this particular job.
    atomic_int_fast16_t child_run_count;
    /// A index into the job cache which points to the parent of this job. Used for linking jobs to
    /// each other, so multiple jobs can be executed via one job.
    atomic_uint_fast16_t parent;
    uint32_t idx;
} job_t;

/**
 Per thread state.
 */
typedef struct RPE_ALIGNAS(JOB_QUEUE_CACHELINE_SIZE) ThreadInfo
{
    /// The work stealing queue for this thread.
    work_stealing_queue_t work_queue;
    /// A thread reference - used for joining upon destruction.
    thread_t* thread;
    /// States whether this thread is joinbale - adopted threads are not joinable.
    bool is_joinable;
    /// A pointer to the job queue this thread is associated with.
    job_queue_t* job_queue;
    /// Random number generator used for generating random thread ids when stealing.
    xoro_rand_t rand_gen;
} thread_info_t;

typedef struct JobQueue
{
    /// An array of jobs which are assigned when creating a job.
    job_t* job_cache;
    /// The number of jobs currently allocated. Used job indices are found in the `free_job_list`
    atomic_int job_count;
    /// Job indices that have complete and can be re-used.
    arena_dyn_array_t free_job_list;
    /// Main thread state cache array.
    thread_info_t thread_states[JOB_QUEUE_MAX_THREAD_COUNT];
    /// The number of threads this job queue is running. Doesn't include adopted threads.
    uint32_t thread_count;
    /// Condition for waiting on for jobs to finish or the threads exiting on destruction.
    cond_wait_t wait_cond;
    /// The number of active jobs.
    atomic_int active_job_count;
    /// State used to determine if the threads should be terminated.
    atomic_bool exit_thread;
    /// The number of adopted threads.
    atomic_int adopted_thread_count;
    /// A map of thread ids and their index in the thread state buffer.
    hash_set_t thread_map;
    /// A mutex used for accessing the thread map.
    mutex_t thread_map_mutex;
    /// A mutex used for the wait condition.
    mutex_t wait_mutex;
    /// The arena used for allocations for this job queue.
    arena_t* arena;
} job_queue_t;

/**
 Initialise a new job queue instance. This will create the stated number of threads, which will be
 ready to accept jobs.
 @param arena The arena to use for allocations.
 @param num_threads The number of thread pools to initialise. If zero, the number of threads will be
 the maximum that can be created on the running system.
 @return A pointer to a new job queue instance.
 */
job_queue_t* job_queue_init(arena_t* arena, uint32_t num_threads);

/**
 Creates a new job instance.
 @param jq A pointer to the job queue.
 @param func A function pointer to execute for this job.
 @param args A void pointer which contains the argument to pass to the function.
 @param parent A pointer to a job which will be a parent to this one. This can be used to link jobs
 and runAndWait can be called on one job.
 @return
 */
job_t* job_queue_create_job(job_queue_t* jq, job_func_t func, void* args, job_t* parent);

/**
  Create a parent job.
  Note: Don't use the same parent job for subsequent runs. Instead create a new parent each time.
  */
job_t* job_queue_create_parent_job(job_queue_t* jq);

/**
 Destroy the specified job queue. This terminates all running thread pools.
 @param jq A pointer to the job qeuue.
 */
void job_queue_destroy(job_queue_t* jq);

/**
 Run a specified job on the job queue.
 @param jq A pointer to the job queue.
 @param job The job to run.
 */
void job_queue_run_job(job_queue_t* jq, job_t* job);

void job_queue_run_ref_job(job_queue_t* jq, job_t* job);

/**
 Run a specified job and wait for completion.
 @param jq A pointer to the job queue.
 @param job The job to run.
 */
void job_queue_run_and_wait(job_queue_t* jq, job_t* job);

/**
 Wait for a specified job to complete.
 @param jq A pointer to the job queue.
 @param job The job to wait on.
 */
void job_queue_wait_and_release(job_queue_t* jq, job_t* job);

/**
 Add a thread to the job queue. Must be a thread which isn't already owned by the queue - usually
 used to adopt the main thread.
 @param jq A pointer to the job queue.
 */
void job_queue_adopt_thread(job_queue_t* jq);

uint32_t job_get_tid(job_t* job);

#endif