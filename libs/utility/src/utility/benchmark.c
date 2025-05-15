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

#include "benchmark.h"

#include "utility/maths.h"

#include <malloc.h>
#include <string.h>

benchmark_t benchmark_ms;

enum Colours
{
    RESET,
    GREEN,
    RED
};
const char* colours[] = {"\033[0m", "\033[32m", "\033[31m"};

int64_t get_time_ns()
{
#if __linux__
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);

    clockid_t cid = CLOCK_REALTIME;
    clock_gettime(cid, &ts);
    return (int64_t)ts.tv_sec * 1000 * 1000 * 1000 + ts.tv_nsec;
#elif WIN32

#endif
}

void alloc_instance(bm_instance_t* i)
{
    size_t idx = benchmark_ms.instance_count++;
    benchmark_ms.instances = realloc(benchmark_ms.instances, sizeof(bm_instance_t) * benchmark_ms.instance_count);
    benchmark_ms.instances[idx] = i;
}

void bm_instance_register_arg1(BM_Func func, const char* name, int64_t arg1)
{
    bm_instance_t* i = malloc(sizeof(bm_instance_t));
    i->args[0] = arg1;
    i->arg_count = 1;
    i->func = func;
    i->name = malloc(strlen(name) + 1);
    strcpy(i->name, name);
    alloc_instance(i);
}

void bm_instance_register_arg2(BM_Func func, const char* name, int64_t arg1, int64_t arg2)
{
    bm_instance_t* i = malloc(sizeof(bm_instance_t));
    i->args[0] = arg1;
    i->args[1] = arg2;
    i->arg_count = 2;
    i->func = func;
    i->name = malloc(strlen(name) + 1);
    strcpy(i->name, name);
    alloc_instance(i);
}

void bm_instance_register_arg3(
    BM_Func func, const char* name, int64_t arg1, int64_t arg2, int64_t arg3)
{
    bm_instance_t* i = malloc(sizeof(bm_instance_t));
    i->args[0] = arg1;
    i->args[1] = arg2;
    i->args[2] = arg3;
    i->arg_count = 3;
    i->func = func;
    i->name = malloc(strlen(name) + 1);
    strcpy(i->name, name);
    alloc_instance(i);
}

bool bm_state_set_running(bm_run_state_t* rs)
{
    int64_t curr_sample = rs->sample++;
    rs->ns[curr_sample] = get_time_ns();
    return curr_sample < rs->size ? true : false;
}

bool bm_run_instance(
    bm_instance_t* instance,
    int64_t* best_avg_ns,
    double* best_conf,
    double* best_dev,
    const int64_t* arg)
{
    assert(instance);

    printf("%s[RUN           ]%s %s", colours[GREEN], colours[RESET], instance->name);
    if (arg)
    {
        printf("(%zu)", *arg);
    }
    printf("\n");

    int64_t ns[BM_MAX_ITERATIONS + 1];
    bm_run_state_t state = {.ns = ns, .sample = 0, .size = 1};
    state.arg = arg ? *arg : 0;

    instance->func(&state);

    int64_t iters = (100 * 1000 * 1000) / ((ns[0] <= ns[1]) ? 1 : ns[1] - ns[0]);
    iters = CLAMP(iters, BM_MIN_ITERATIONS, BM_MAX_ITERATIONS);

    int midx = 0;
    bool res = true;
    for (; midx < 100 && res; ++midx)
    {
        int64_t avg_ns = 0;
        double conf = 0.0;
        double dev = 0.0;

        iters *= (int64_t)midx + 1;
        iters = MIN(iters, BM_MAX_ITERATIONS);

        state.size = iters;
        state.sample = 0;
        instance->func(&state);

        for (int kidx = 0; kidx < iters; ++kidx)
        {
            ns[kidx] = ns[kidx + 1] - ns[kidx];
        }
        for (int kidx = 0; kidx < iters; ++kidx)
        {
            avg_ns += ns[kidx];
        }
        avg_ns /= iters;

        for (int kidx = 0; kidx < iters; ++kidx)
        {
            double v = (double)(ns[kidx] - avg_ns);
            dev += v * v;
        }
        dev = sqrtf((float)dev / (float)iters);
        dev = (dev / (double)avg_ns) * 100.0;

        conf = 2.576 * dev / sqrtf((float)iters);
        conf = (conf / (double)avg_ns) * 100.0;

        res = conf > benchmark_ms.confidence;

        if (conf < *best_conf)
        {
            *best_avg_ns = avg_ns;
            *best_dev = dev;
            *best_conf = conf;
        }
    }
    return res;
}

void bm_report_results(
    bm_instance_t* instance, bool is_confident, int64_t best_avg_ns, double best_conf)
{
    const char* colour = is_confident ? colours[RED] : colours[GREEN];
    const char* status = is_confident ? "[    FAILED    ]" : "[         OK   ]";

    printf("%s%s%s %s (mean ", colour, status, colours[RESET], instance->name);

    const char* unit = "us";
    for (int i = 0; i < 2; ++i)
    {
        if (best_avg_ns <= 1000000)
        {
            break;
        }
        best_avg_ns /= 1000;
        switch (i)
        {
            case 0:
                unit = "ms";
                break;
            case 1:
                unit = "s";
                break;
        }
    }

    printf(
        "%zu.%03zu%s, confidence +- %f%%)\n",
        best_avg_ns / 1000,
        best_avg_ns % 1000,
        unit,
        best_conf);
}

void bm_run_benchmarks()
{
    printf(
        "%s[==============]%s Running %zu benchmarks.\n",
        colours[GREEN],
        colours[RESET],
        benchmark_ms.instance_count);

    size_t failed_count = 0;
    size_t* failed_bms = NULL;

    for (size_t idx = 0; idx < benchmark_ms.instance_count; ++idx)
    {
        int64_t best_avg_ns = 0;
        double best_dev = 0.0;
        double best_conf = 101.0;

        bm_instance_t* instance = benchmark_ms.instances[idx];

        size_t run_count = instance->arg_count > 0 ? instance->arg_count : 1;
        for (size_t arg_idx = 0; arg_idx < run_count; ++arg_idx)
        {
            int64_t* arg = instance->arg_count > 0 ? &instance->args[arg_idx] : NULL;
            bool res = bm_run_instance(instance, &best_avg_ns, &best_conf, &best_dev, arg);
            bm_report_results(instance, res, best_avg_ns, best_conf);

            if (res)
            {
                size_t fail_idx = failed_count++;
                failed_bms = (size_t*)realloc((void*)failed_bms, sizeof(size_t) * failed_count);
                failed_bms[fail_idx] = idx;
            }
        }
    }

    printf(
        "%s[    PASSED    ]%s %zu benchmarks.\n",
        colours[GREEN],
        colours[RESET],
        benchmark_ms.instance_count - failed_count);
    if (failed_count > 0)
    {
        printf(
            "%s[    FAILED    ]%s %zu benchmarks.\n",
            colours[RED],
            colours[RESET],
            failed_count);
        for (size_t i = 0; i < failed_count; ++i)
        {
            printf(
                "%s[    FAILED    ]%s %s\n",
                colours[RED],
                colours[RESET],
                benchmark_ms.instances[failed_bms[i]]->name);
        }
    }
}

void bm_init()
{
    benchmark_ms.confidence = 2.5;
}

void bm_shutdown() {}