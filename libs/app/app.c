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

#include "app.h"
#include "nk_helper.h"
#include "camera_view.h"

#include <rpe/renderer.h>
#include <utility/sleep.h>

int rpe_app_init(const char* win_title, uint32_t win_width, uint32_t win_height, rpe_app_t* new_app, rpe_settings_t* settings, bool show_ui)
{
    new_app->should_close = false;
    new_app->camera_near = 0.1f;
    new_app->camera_far = 100.0f;
    new_app->camera_fov = 60.0f;

    // Create a small arena for the application.
    int arena_err = arena_new(RPE_APP_ARENA_SIZE, &new_app->arena);
    if (arena_err != ARENA_SUCCESS)
    {
        return 1;
    }

    int err =
        app_window_init(new_app, win_title, win_width, win_height, &new_app->window, settings, show_ui);

    return err;
}

void rpe_app_shutdown(rpe_app_t* app)
{
    assert(app);

    rpe_engine_shutdown(app->engine);
    app_window_shutdown(&app->window, app);
}

void rpe_app_run(
    rpe_app_t* app,
    rpe_renderer_t* renderer,
    PreRenderFunc pre_render_func,
    void* pre_data,
    PostRenderFunc post_render_func,
    void* post_data,
    UiCallback ui_callback)
{
    assert(app);
    assert(renderer);

    app_window_t* app_win = &app->window;

    while (!app->should_close)
    {
        // check for any input from the window
        app_window_poll();
        app->should_close = glfwWindowShouldClose(app_win->glfw_window);

        if (app_win->show_ui)
        {
            nk_helper_new_frame(app_win->nk, app_win);
            nk_helper_render(app_win->nk, app->engine, app_win, ui_callback);
        }

        double now = glfwGetTime();
        double time_step = app->prev_time > 0.0 ? now - app->prev_time : 1.0 / 60.0;
        app->prev_time = now;

        // update the camera if any changes in key state have been detected
        rpe_camera_view_update_key_events(&app_win->cam_view, (float)time_step);
        rpe_camera_set_view_matrix(app_win->camera, &app_win->cam_view.view);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        long delay_ms =
            mode && mode->refreshRate ? (long)roundf((float)1000 / (float)mode->refreshRate) : 16;
        msleep(delay_ms);

        rpe_renderer_begin_frame(renderer);

        if (pre_render_func)
        {
            pre_render_func(app->engine, pre_data);
        }

        // begin the rendering for this frame - render the main scene
        rpe_renderer_render(renderer, app->scene, false);

        if (app_win->show_ui)
        {
            rpe_renderer_render(renderer, app_win->nk->scene, false);
        }

        if (post_render_func)
        {
            post_render_func(app->engine, post_data);
        }

        rpe_renderer_end_frame(renderer);

    }
}