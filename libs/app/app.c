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

int rpe_app_init(const char* win_title, uint32_t win_width, uint32_t win_height, rpe_app_t* new_app)
{
    int err = app_window_init(new_app, win_title, win_width, win_height, &new_app->window);
    new_app->should_close = false;
    return err;
}

void rpe_app_shutdown(rpe_app_t* app)
{
    rpe_engine_shutdown(app->engine);
    app_window_shutdown(&app->window);
}

void rpe_app_run(rpe_app_t* app)
{
    while (!app->should_close)
    {
        // check for any input from the window
        app_window_poll();

        app->should_close = glfwWindowShouldClose(app->window.glfw_window);
    }
}