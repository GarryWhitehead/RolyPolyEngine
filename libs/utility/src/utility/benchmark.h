/* Copyright (c) 2024-2025 Garry Whitehead
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

#ifndef __UTIL_BENCHMARK_H__
#define __UTIL_BENCHMARK_H__

#include <stdbool.h>
#include <stdint.h>

#if __linux__
#include <syscall.h>
#include <time.h>
#include <unistd.h>
#elif WIN32

#endif

#define BM_MAX_ARG_COUNT 3
#define BM_MIN_ITERATIONS 10
#define BM_MAX_ITERATIONS 1000

typedef struct BenchmarkRunState
{
    int64_t* ns;
    int64_t size;
    int64_t sample;
    int64_t arg;
} bm_run_state_t;

typedef void (*BM_Func)(bm_run_state_t*);

typedef struct BenchmarkInstance
{
    BM_Func func;
    char* name;
    int64_t args[BM_MAX_ARG_COUNT];
    uint32_t arg_count;
} bm_instance_t;

typedef struct BenchMark
{
    bm_instance_t** instances;
    size_t instance_count;
    double confidence;
} benchmark_t;

extern benchmark_t benchmark_ms;

/**
 Instance registration functions.
 */
void bm_instance_register_arg1(BM_Func func, const char* name, int64_t arg1);
void bm_instance_register_arg2(BM_Func func, const char* name, int64_t arg1, int64_t arg2);
void bm_instance_register_arg3(
    BM_Func func, const char* name, int64_t arg1, int64_t arg2, int64_t arg3);

/**
 Benchmark main functions.
 */
void bm_init();

void bm_run_benchmarks();

void bm_shutdown();

/**
 Benchmark state functions.
 */
bool bm_state_set_running(bm_run_state_t* rs);

#ifdef __clang__
#define BM_DONT_OPTIMISE(val) asm volatile("" : "+r,m"(val) : : "memory");
#else
#define BM_DONT_OPTIMISE(val) asm volatile("" : "+m,r"(val) : : "memory");
#endif

#ifdef __COUNTER__
#define BM_UNIQUE_ID __COUNTER__
#else
#define BENCHMARK_PRIVATE_UNIQUE_ID __LINE__
#endif

#define BENCHMARK_PRIVATE_CONCAT2(a, b, c) a##b##c
#define BENCHMARK_PRIVATE_CONCAT(a, b, c) BENCHMARK_PRIVATE_CONCAT2(a, b, c)

#define BM_GENERATE_NAME(...) BENCHMARK_PRIVATE_CONCAT(benchmark_uniq_, BM_UNIQUE_ID, __VA_ARGS__)

#define BM_INSTANCE_NAME(n) static bm_instance_t* BM_GENERATE_NAME(n)

#define BM_INITIALISER(func) static void bm_runner_##func(void) __attribute__((constructor));

#define BENCHMARK_ARG1(func, a0)                                                                   \
    BM_INITIALISER(func)                                                                           \
    static void bm_runner_##func() { bm_instance_register_arg1(func, #func, a0); }
#define BENCHMARK_ARG2(func, a0, a1)                                                               \
    BM_INITIALISER(func)                                                                           \
    static void bm_runner_##func() { bm_instance_register_arg2(func, #func, a0, a1); }
#define BENCHMARK_ARG3(func, a0, a1, a2)                                                           \
    BM_INITIALISER(func)                                                                           \
    static void bm_runner_##func() { bm_instance_register_arg3(func, #func, a0, a1, a2); }

#define BENCHMARK_MAIN()                                                                           \
    int main(int argc, char** argv)                                                                \
    {                                                                                              \
        bm_init();                                                                                 \
        bm_run_benchmarks();                                                                       \
        bm_shutdown();                                                                             \
        return 0;                                                                                  \
    }


#endif