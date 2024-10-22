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

#include "job_queue.h"

#include <log.h>
#include <utility/arena.h>
#include <utility/hash.h>

#define _GNU_SOURCE
#include <assert.h>
#include <math.h>
#ifdef __linux__
#include <sched.h>
#elif WIN32
#include <processthreadsapi.h>
#endif
#include <string.h>

uint32_t _get_cpu_count()
{
    uint32_t count = 0;
#ifdef __linux__
    cpu_set_t cpu_set;
    sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);
    count = CPU_COUNT(&cpu_set);
#elif WIN32
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    count = sys_info.dwNumberOfProcessors;
#endif
    return count;
}

void _set_thread_name(const char* name)
{
#ifdef __linux__
    pthread_setname_np(pthread_self(), name);
#elif WIN32
#endif
}

void _thread_affinity_by_id(size_t id)
{
#ifdef __linux__
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(id, &cpu_set);
    sched_setaffinity(gettid(), sizeof(cpu_set_t), &cpu_set);
#endif
}

uint32_t _get_thread_id()
{
#if __linux__
    return (uint32_t)gettid();
#elif WIN32
    return (uint32_t)GetCurrentThreadId();
#endif
    return 0;
}

bool _active_jobs(job_queue_t* jq)
{
    int active_count = atomic_load_explicit(&jq->active_job_count, memory_order_relaxed);
    return active_count > 0;
}

bool _job_completed(job_t* job)
{
    int count = atomic_load_explicit(&job->run_count, memory_order_acquire);
    return count <= 0;
}

bool _exit_requested(thread_info_t* info)
{
    return atomic_load_explicit(&info->job_queue->exit_thread, memory_order_relaxed);
}

void _decrement_ref(job_queue_t* jq, job_t* job)
{
    assert(job);
    atomic_uint_fast16_t count =
        atomic_fetch_sub_explicit(&job->ref_count, 1, memory_order_acq_rel);
    assert(count > 0);
    if (count == 1)
    {
        // TODO: delete job from array.
    }
}

void _wake(job_queue_t* jq, int count)
{
    mutex_lock(&jq->wait_mutex);
    if (count == 1)
    {
        condition_signal(&jq->wait_cond);
    }
    else
    {
        condition_brdcast(&jq->wait_cond);
    }
    mutex_unlock(&jq->wait_mutex);
}

void _wake_all(job_queue_t* jq)
{
    mutex_lock(&jq->wait_mutex);
    condition_brdcast(&jq->wait_cond);
    mutex_unlock(&jq->wait_mutex);
}

void _wait(job_queue_t* jq) { condition_wait(&jq->wait_cond, &jq->wait_mutex); }

job_t* _pop(job_queue_t* jq, thread_info_t* info)
{
    atomic_fetch_sub_explicit(&jq->active_job_count, 1, memory_order_relaxed);

    int* idx_ptr = work_stealing_queue_pop(&info->work_queue);
    job_t* job = NULL;
    if (idx_ptr)
    {
        int idx = *idx_ptr;
        assert(idx >= 0);
        job = DYN_ARRAY_GET_PTR(job_t, &jq->job_cache, idx - 1);
    }

    if (!job)
    {
        atomic_int old_job_count =
            atomic_fetch_add_explicit(&jq->active_job_count, 1, memory_order_relaxed);
        // Wake up other threads if there are still active jobs.
        if (old_job_count >= 0)
        {
            _wake(jq, old_job_count + 1);
        }
    }
    return job;
}

void _push(job_queue_t* jq, thread_info_t* info, job_t* job)
{
    assert(job);

    size_t job_idx = job - (job_t*)jq->job_cache.data;
    assert(job_idx >= 0);
    job_idx += 1;
    WORK_STEALING_QUEUE_PUSH(&info->work_queue, (int*)&job_idx);

    atomic_int old_job_count =
        atomic_fetch_add_explicit(&jq->active_job_count, 1, memory_order_relaxed);
    // If another thread has taken this job (job count will be negative) then don't wake the thread
    // as we no longer have any work.
    if (old_job_count >= 0)
    {
        _wake(jq, old_job_count + 1);
    }
}

job_t* _steal_from_queue(job_queue_t* jq, work_stealing_queue_t* queue)
{
    atomic_fetch_sub_explicit(&jq->active_job_count, 1, memory_order_relaxed);

    int* idx_ptr = work_stealing_queue_steal(queue);
    job_t* job = NULL;
    if (idx_ptr)
    {
        int idx = *idx_ptr;
        assert(idx >= 0);
        job = DYN_ARRAY_GET_PTR(job_t, &jq->job_cache, idx - 1);
    }

    if (!job)
    {
        atomic_int old_job_count =
            atomic_fetch_add_explicit(&jq->active_job_count, 1, memory_order_relaxed);
        // Wake up other threads if there are still active jobs.
        if (old_job_count >= 0)
        {
            _wake(jq, old_job_count + 1);
        }
    }
    return job;
}

job_t* _steal_from_state(job_queue_t* jq, thread_info_t* info)
{
    job_t* job = NULL;
    do
    {
        thread_info_t* steal_info = NULL;
        int adopted_count = atomic_load_explicit(&jq->adopted_thread_count, memory_order_relaxed);
        uint32_t thread_count = jq->thread_count + adopted_count;
        if (thread_count >= 2)
        {
            do
            {
                uint64_t rand = xoro_rand_next(&info->rand_gen) % thread_count;
                steal_info = &jq->thread_states[rand];
            } while (steal_info == info);
        }
        if (steal_info)
        {
            job = _steal_from_queue(jq, &steal_info->work_queue);
        }
    } while (!job && _active_jobs(jq));

    return job;
}

void _thread_finish(thread_info_t* info, job_t* job)
{
    bool wake_threads = false;
    do
    {
        atomic_uint_fast16_t count =
            atomic_fetch_sub_explicit(&job->run_count, 1, memory_order_relaxed);
        if (count == 1)
        {
            job_t* parent_job = job->parent == UINT16_MAX
                ? NULL
                : DYN_ARRAY_GET_PTR(job_t, &info->job_queue->job_cache, job->parent);
            _decrement_ref(info->job_queue, job);
            job = parent_job;
            wake_threads = true;
        }
        else
        {
            break;
        }
    } while (job);

    if (wake_threads)
    {
        _wake_all(info->job_queue);
    }
}

job_t* _thread_execute(thread_info_t* info)
{
    job_t* job = _pop(info->job_queue, info);
    // If no more jobs left in the queue, try to steal a job from another thread.
    if (!job)
    {
        job = _steal_from_state(info->job_queue, info);
    }
    if (job)
    {
        assert(job->func);
        job->func(job->args);
        _thread_finish(info, job);
    }
    return job;
}

void* _thread_loop(void* arg)
{
    _set_thread_name("job_queue.loop");

    thread_info_t* info = (thread_info_t*)arg;
    mutex_lock(&info->job_queue->thread_map_mutex);
    uint32_t id = _get_thread_id();
    HASH_SET_INSERT(&info->job_queue->thread_map, &id, &info);
    mutex_unlock(&info->job_queue->thread_map_mutex);

    do
    {
        job_t* job = _thread_execute(info);
        if (!job)
        {
            mutex_lock(&info->job_queue->wait_mutex);
            // Keep waiting until either an exit from the thread is requested or a new job is
            // pushed.
            while (!_exit_requested(info) && !_active_jobs(info->job_queue))
            {
                _wait(info->job_queue);
            }
            mutex_unlock(&info->job_queue->wait_mutex);
        }
    } while (!_exit_requested(info));

    return NULL;
}

job_queue_t* job_queue_init(arena_t* arena, uint32_t num_threads, uint32_t num_adpoted_threads)
{
    assert(arena);
    assert(num_threads < JOB_QUEUE_MAX_THREAD_COUNT);
    job_queue_t* jq = ARENA_MAKE_ZERO_STRUCT(arena, job_queue_t);

    jq->thread_count = num_threads;
    jq->active_job_count = 0;
    jq->adopted_thread_count = 0;
    jq->exit_thread = false;
    jq->thread_map = HASH_SET_CREATE(uint32_t, thread_info_t*, arena, &murmur_hash3);
    jq->arena = arena;

    // static_assert(atomic_is_lock_free(&jq->exit_thread), "Bool isn't lockless.");

    if (!num_threads)
    {
        jq->thread_count = _get_cpu_count();
    }
    jq->thread_count = fmax(1, fmin(JOB_QUEUE_MAX_THREAD_COUNT, jq->thread_count));

    mutex_init(&jq->thread_map_mutex);
    mutex_init(&jq->wait_mutex);
    condition_init(&jq->wait_cond);

    int err = MAKE_DYN_ARRAY(job_t, arena, 100, &jq->job_cache);
    assert(err == ARENA_SUCCESS);

    for (uint32_t i = 0; i < jq->thread_count; ++i)
    {
        thread_info_t* info = &jq->thread_states[i];
        info->job_queue = jq;
        info->is_joinable = true;
        info->work_queue = WORK_STEALING_DEQUE_INIT(int, arena, JOB_QUEUE_MAX_JOB_COUNT);
        info->rand_gen = xoro_rand_init(_get_thread_id(), 0x1234);
        info->thread = thread_create(&_thread_loop, info, arena);
    }
    return jq;
}

job_t* job_queue_create_job(job_queue_t* jq, job_func_t func, void* args, job_t* parent)
{
    assert(func);

    job_t job;
    job.func = func;
    job.args = args;
    job.ref_count = 1;
    job.run_count = 1;
    job.parent = UINT16_MAX;
    if (parent)
    {
        atomic_uint_fast16_t count =
            atomic_fetch_add_explicit(&parent->run_count, 1, memory_order_relaxed);
        assert(count > 0);
        job.parent = parent - (job_t*)jq->job_cache.data;
    }

    return DYN_ARRAY_APPEND(&jq->job_cache, &job);
}

void job_queue_destroy(job_queue_t* jq)
{
    atomic_store(&jq->exit_thread, true);
    mutex_lock(&jq->wait_mutex);
    condition_brdcast(&jq->wait_cond);
    mutex_unlock(&jq->wait_mutex);

    for (int i = 0; i < jq->thread_count; ++i)
    {
        thread_info_t* info = &jq->thread_states[i];
        if (info->is_joinable)
        {
            thread_join(info->thread);
        }
    }

    mutex_destroy(&jq->wait_mutex);
    condition_destroy(&jq->wait_cond);
    mutex_destroy(&jq->thread_map_mutex);
}

void job_queue_run_job(job_queue_t* jq, job_t* job)
{
    assert(job);

    mutex_lock(&jq->thread_map_mutex);
    uint32_t id = _get_thread_id();
    thread_info_t** info = HASH_SET_GET(&jq->thread_map, &id);
    assert(info && "Trying to run on a thread that hasn't been adopted?");
    mutex_unlock(&jq->thread_map_mutex);

    _push(jq, *info, job);
}

void job_queue_run_and_wait(job_queue_t* jq, job_t* job)
{
    assert(job);

    job_t* retain_job = job;
    atomic_fetch_add_explicit(&retain_job->ref_count, 1, memory_order_relaxed);
    job_queue_run_job(jq, job);
    job_queue_wait_and_release(jq, job);
}

void job_queue_wait_and_release(job_queue_t* jq, job_t* job)
{
    assert(job);

    uint32_t id = _get_thread_id();
    mutex_lock(&jq->thread_map_mutex);
    thread_info_t** info = HASH_SET_GET(&jq->thread_map, &id);
    mutex_unlock(&jq->thread_map_mutex);
    assert(info);

    do
    {
        if (!_thread_execute(*info))
        {
            if (_job_completed(job))
            {
                break;
            }
            mutex_lock(&jq->wait_mutex);
            if (!_job_completed(job) && !_exit_requested(*info) && !_active_jobs(jq))
            {
                _wait(jq);
            }
            mutex_unlock(&jq->wait_mutex);
        }
    } while (!_job_completed(job) && !_exit_requested(*info));

    _decrement_ref(jq, job);
    job = NULL;
}

void job_queue_adopt_thread(job_queue_t* jq)
{
    uint32_t id = _get_thread_id();
    mutex_lock(&jq->thread_map_mutex);
    thread_info_t* info = HASH_SET_GET(&jq->thread_map, &id);
    mutex_unlock(&jq->thread_map_mutex);

    if (info && info->job_queue == jq)
    {
        log_warn("This thread has already been adopted by this job queue.");
        return;
    }

    atomic_int adopted_count =
        atomic_fetch_add_explicit(&jq->adopted_thread_count, 1, memory_order_relaxed);
    assert(adopted_count + jq->thread_count < JOB_QUEUE_MAX_THREAD_COUNT);

    info = &jq->thread_states[adopted_count + jq->thread_count];
    info->job_queue = jq;
    info->is_joinable = false;
    info->work_queue = WORK_STEALING_DEQUE_INIT(uint32_t, jq->arena, JOB_QUEUE_MAX_JOB_COUNT);
    info->rand_gen = xoro_rand_init(_get_thread_id(), 0x1234);

    mutex_lock(&jq->thread_map_mutex);
    HASH_SET_INSERT(&jq->thread_map, &id, &info);
    mutex_unlock(&jq->thread_map_mutex);
}