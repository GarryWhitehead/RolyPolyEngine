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

#ifndef __APP_NK_HELPER_H__
#define __APP_NK_HELPER_H__

#include <rpe/object.h>
#include <utility/arena.h>

#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_STANDARD_IO
#include <nuklear.h>

#define RPE_NK_HELPER_MAX_RENDERABLES 10
#define RPE_NK_HELPER_TEXT_MAX 256
#define RPE_NK_HELPER_DOUBLE_CLICK_LO 0.02
#define RPE_NK_HELPER_DOUBLE_CLICK_HI 0.2

typedef struct Scene rpe_scene_t;
typedef struct Engine rpe_engine_t;
typedef struct AppWindow app_window_t;
typedef struct Camera rpe_camera_t;
typedef struct Renderable rpe_renderable_t;
typedef struct Material rpe_material_t;

typedef void (*UiCallback)(rpe_engine_t*, app_window_t*);

typedef struct NkInstance
{
    struct nk_context ctx;
    struct nk_font_atlas atlas;
    struct nk_draw_null_texture tex_null;
    unsigned int text[RPE_NK_HELPER_TEXT_MAX];
    int text_len;
    nk_char key_events[NK_KEY_MAX];
    struct nk_vec2 scroll;
    struct nk_vec2 fb_scale;
    double last_button_click;
    int is_double_click_down;
    struct nk_vec2 double_click_pos;
    float delta_time_seconds_last;

    struct nk_buffer v_buffer;
    struct nk_buffer i_buffer;
    struct nk_buffer cmds;

    rpe_scene_t* scene;
    rpe_camera_t* camera;
    rpe_object_t rend_objs[RPE_NK_HELPER_MAX_RENDERABLES];
    rpe_renderable_t* renderables[RPE_NK_HELPER_MAX_RENDERABLES];
    rpe_object_t transform_obj;
    rpe_material_t* font_mat;
} nk_instance_t;

nk_instance_t*
nk_helper_init(const char* font_path, int font_size, rpe_engine_t* engine, arena_t* arena);

void nk_helper_destroy(nk_instance_t* nk);

void nk_helper_new_frame(nk_instance_t* nk, app_window_t* win);

void nk_helper_render(
    nk_instance_t* nk, rpe_engine_t* engine, app_window_t* win, UiCallback ui_callback);

#endif