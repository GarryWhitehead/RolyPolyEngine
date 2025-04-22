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

#ifndef __APP_APP_H__
#define __APP_APP_H__

#include "window.h"
#include "nk_helper.h"

#include <rpe/engine.h>
#include <rpe/scene.h>

#define RPE_APP_ARENA_SIZE 1 << 30

typedef struct Engine rpe_engine_t;

typedef void (*PreRenderFunc)(rpe_engine_t*, void*);
typedef void (*PostRenderFunc)(rpe_engine_t*, void*);

typedef struct Application
{
    app_window_t window;
    rpe_engine_t* engine;
    rpe_scene_t* scene;
    vkapi_driver_t* driver;
    arena_t arena;

    bool should_close;
    
    double prev_time;

    // camera paramters
    float camera_fov;
    float camera_near;
    float camera_far;

} rpe_app_t;

/**
 Initialise a new application instance.
 This sets up the Vulkan and RPE environment, creating a new window for drawing to.
 @param win_title The title of the window.
 @param win_width The width of the window. If zero, and @sa win_height, then a fullscreen borderless
 window will be created.
 @param win_height The width of the height. If zero, and @sa win_width, then a fullscreen borderless
 window will be created.
 @param new_app A pointer to the app object which will be initialised.
 @returns an error code.
 */
int rpe_app_init(
    const char* win_title, uint32_t win_width, uint32_t win_height, rpe_app_t* new_app, rpe_settings_t* settings, bool show_ui);

/**
 Destroy all resource associated with this app - this will terminate all Vulkan and RPE resources.
 @param app A pointer to a app object.
 */
void rpe_app_shutdown(rpe_app_t* app);

/**
 This begins the main engine loop. This will loop until either the window is closed or the `esc` key
 is pressed.
 @param app A pointer to a initialised app object.
 */
void rpe_app_run(
    rpe_app_t* app,
    rpe_renderer_t* renderer,
    PreRenderFunc pre_render_func,
    void* pre_data,
    PostRenderFunc post_render_func,
    void* post_data,
    UiCallback ui_callback);

#endif