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
#ifndef __RPE_SETTINGS_H__
#define __RPE_SETTINGS_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct SystemSettings
{
    /// The maximum number of models that will be drawn in a single scene draw. If the model count
    /// exceeds this value, then the remaining objects will not be drawn. Increasing this value will
    /// lead to higher GPU memory usage.
    size_t max_static_model_draw_count;
    size_t scratch_arena_size;
    size_t frame_arena_size;
    size_t perm_arena_size;
};

struct ShadowSettings
{
    uint32_t cascade_dims;
    int cascade_count;
    float split_lambda;
    bool enable_debug_cascade;
    uint32_t debug_cascade_idx;
};

struct Settings
{
    uint32_t gbuffer_dims;
    bool draw_shadows;

    struct ShadowSettings shadow;
    struct SystemSettings system;
};

static inline struct Settings rpe_init_settings()
{
    struct Settings settings = {0};
    settings.gbuffer_dims = 2048;
    settings.draw_shadows = true;

    struct ShadowSettings* shadow_settings = &settings.shadow;
    shadow_settings->cascade_count = 4;
    shadow_settings->split_lambda = 0.9f;
    shadow_settings->cascade_dims = 2048;

    struct SystemSettings* system_settings = &settings.system;
    system_settings->max_static_model_draw_count = 10000;
    system_settings->scratch_arena_size = 1 << 25;
    system_settings->perm_arena_size = 1 << 30;
    system_settings->frame_arena_size = 1 << 30;

    return settings;
}

#endif