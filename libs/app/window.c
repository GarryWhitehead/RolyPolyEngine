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

#include "window.h"

#include "app.h"
#include "nk_helper.h"

#include <rpe/engine.h>
#include <rpe/settings.h>
#include <string.h>
#include <utility/arena.h>
#include <vulkan-api/driver.h>
#include <vulkan-api/error_codes.h>

int app_window_init(
    rpe_app_t* app,
    const char* title,
    uint32_t width,
    uint32_t height,
    app_window_t* new_win,
    rpe_settings_t* settings,
    bool show_ui)
{
    memset(new_win, 0, sizeof(app_window_t));
    new_win->show_ui = show_ui;
    new_win->width = width;
    new_win->height = height;

    if (!glfwInit())
    {
        return APP_ERROR_GLFW_NOT_INIT;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // If no title specified, no window decorations will be used.
    if (!title)
    {
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    }

    // if dimensions set to zero, get the primary monitor which will create a
    // fullscreen, borderless window
    if (width == 0 && height == 0)
    {
        new_win->glfw_monitor = glfwGetPrimaryMonitor();
        new_win->glfw_vmode = glfwGetVideoMode(new_win->glfw_monitor);
        width = new_win->glfw_vmode->width;
        height = new_win->glfw_vmode->height;
    }

    new_win->glfw_window =
        glfwCreateWindow((int)width, (int)height, title, new_win->glfw_monitor, NULL);
    if (!new_win->glfw_window)
    {
        return APP_ERROR_NO_WINDOW;
    }

    // set window inputs
    glfwSetWindowUserPointer(new_win->glfw_window, new_win);
    glfwSetKeyCallback(new_win->glfw_window, keyCallback);
    glfwSetInputMode(new_win->glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPosCallback(new_win->glfw_window, mouseCallback);
    glfwSetMouseButtonCallback(new_win->glfw_window, mouseButtonPressCallback);
    glfwSetScrollCallback(new_win->glfw_window, scrollCallback);

    // Create a new vulkan driver instance along with the window surface.
    uint32_t count;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&count);
    assert(glfw_extensions);

    int error_code;
    app->driver = vkapi_driver_init(glfw_extensions, count, &error_code);
    if (error_code != VKAPI_SUCCESS)
    {
        return APP_ERROR_NO_DEVICE;
    }

    // Create the window surface.
    VkResult err = glfwCreateWindowSurface(
        app->driver->context->instance, new_win->glfw_window, VK_NULL_HANDLE, &new_win->vk_surface);
    if (err)
    {
        return APP_ERROR_NO_SURFACE;
    }

    // Create the abstract physical device object.
    error_code = vkapi_driver_create_device(app->driver, new_win->vk_surface);
    if (error_code != VKAPI_SUCCESS)
    {
        return APP_ERROR_NO_DEVICE;
    }

    // create the engine (dependent on the glfw window for creating the device).
    app->engine = rpe_engine_create(app->driver, settings);

    uint32_t g_width, g_height;
    glfwGetWindowSize(app->window.glfw_window, (int*)&g_width, (int*)&g_height);

    new_win->cam_view = rpe_camera_view_init(app->engine);
    new_win->camera = rpe_camera_init(
        app->engine,
        app->camera_fov,
        g_width,
        g_height,
        app->camera_near,
        app->camera_far,
        RPE_PROJECTION_TYPE_PERSPECTIVE);

    // Create a scene for our application.
    app->scene = rpe_engine_create_scene(app->engine);
    rpe_scene_set_current_camera(app->scene, app->engine, new_win->camera);
    rpe_engine_set_current_scene(app->engine, app->scene);

    if (new_win->show_ui)
    {
        const char* font_path = RPE_ASSETS_DIRECTORY "/Roboto-Regular.ttf";
        new_win->nk = nk_helper_init(font_path, 14.0f, app->engine, new_win, &app->arena);
        if (!new_win->nk)
        {
            return APP_ERROR_UI_FONT_NOT_FOUND;
        }
    }
    return APP_SUCCESS;
}

void app_window_shutdown(app_window_t* win, rpe_app_t* app)
{
    assert(win);
    assert(app);

    vkapi_driver_shutdown(app->driver, win->vk_surface);
    nk_helper_destroy(win->nk);
    glfwDestroyWindow(win->glfw_window);
    glfwTerminate();
}

void app_window_poll() { glfwPollEvents(); }

void app_window_key_response(GLFWwindow* window, int key, int scan_code, int action, int mode)
{
    app_window_t* input_sys = (app_window_t*)glfwGetWindowUserPointer(window);
    if (action != GLFW_PRESS && action != GLFW_RELEASE)
    {
        return;
    }

    if (action == GLFW_PRESS)
    {
        if (key == GLFW_KEY_ESCAPE)
        {
            glfwSetWindowShouldClose(window, true);
        }
        else
        {
            rpe_camera_view_key_down_event(
                &input_sys->cam_view, rpe_camera_view_convert_key_code(key));
        }
    }
    else if (action == GLFW_RELEASE)
    {
        rpe_camera_view_key_up_event(&input_sys->cam_view, rpe_camera_view_convert_key_code(key));
    }

    if (input_sys->show_ui)
    {
        nk_char a = (action == GLFW_RELEASE) ? nk_false : nk_true;
        nk_instance_t* nk = input_sys->nk;

        switch (key)
        {
            case GLFW_KEY_DELETE:
                nk->key_events[NK_KEY_DEL] = a;
                break;
            case GLFW_KEY_TAB:
                nk->key_events[NK_KEY_TAB] = a;
                break;
            case GLFW_KEY_BACKSPACE:
                nk->key_events[NK_KEY_BACKSPACE] = a;
                break;
            case GLFW_KEY_UP:
                nk->key_events[NK_KEY_UP] = a;
                break;
            case GLFW_KEY_DOWN:
                nk->key_events[NK_KEY_DOWN] = a;
                break;
            case GLFW_KEY_LEFT:
                nk->key_events[NK_KEY_LEFT] = a;
                break;
            case GLFW_KEY_RIGHT:
                nk->key_events[NK_KEY_RIGHT] = a;
                break;

            case GLFW_KEY_PAGE_UP:
                nk->key_events[NK_KEY_SCROLL_UP] = a;
                break;
            case GLFW_KEY_PAGE_DOWN:
                nk->key_events[NK_KEY_SCROLL_DOWN] = a;
                break;

            case GLFW_KEY_C:
                nk->key_events[NK_KEY_COPY] = a;
                break;
            case GLFW_KEY_V:
                nk->key_events[NK_KEY_PASTE] = a;
                break;
            case GLFW_KEY_X:
                nk->key_events[NK_KEY_CUT] = a;
                break;
            case GLFW_KEY_Z:
                nk->key_events[NK_KEY_TEXT_UNDO] = a;
                break;
            case GLFW_KEY_R:
                nk->key_events[NK_KEY_TEXT_REDO] = a;
                break;
            case GLFW_KEY_B:
                nk->key_events[NK_KEY_TEXT_LINE_START] = a;
                break;
            case GLFW_KEY_E:
                nk->key_events[NK_KEY_TEXT_LINE_END] = a;
                break;
            case GLFW_KEY_A:
                nk->key_events[NK_KEY_TEXT_SELECT_ALL] = a;
                break;

            case GLFW_KEY_ENTER:
            case GLFW_KEY_KP_ENTER:
                nk->key_events[NK_KEY_ENTER] = a;
                break;
            default:;
        }
    }
}

void app_window_mouse_button_response(GLFWwindow* window, int button, int action, int mods)
{
    app_window_t* input_sys = (app_window_t*)glfwGetWindowUserPointer(window);

    // check the left mouse button
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            rpe_camera_view_mouse_button_down(&input_sys->cam_view, xpos, ypos);

            if (input_sys->show_ui)
            {
                nk_instance_t* nk = input_sys->nk;
                double dt = glfwGetTime() - nk->last_button_click;
                if (dt > RPE_NK_HELPER_DOUBLE_CLICK_LO && dt < RPE_NK_HELPER_DOUBLE_CLICK_HI)
                {
                    nk->is_double_click_down = nk_true;
                    nk->double_click_pos = nk_vec2((float)xpos, (float)ypos);
                }
                nk->last_button_click = glfwGetTime();
            }
        }
        else if (action == GLFW_RELEASE)
        {
            rpe_camera_view_mouse_button_up(&input_sys->cam_view);

            if (input_sys->show_ui)
            {
                input_sys->nk->is_double_click_down = nk_false;
            }
        }
    }
}

void app_window_mouse_move_response(GLFWwindow* window, double xpos, double ypos)
{
    app_window_t* input_sys = (app_window_t*)glfwGetWindowUserPointer(window);
    rpe_camera_view_mouse_update(&input_sys->cam_view, xpos, ypos);
}

void app_window_scroll_response(GLFWwindow* window, double xoffset, double yoffset)
{
    app_window_t* input_sys = (app_window_t*)glfwGetWindowUserPointer(window);
    float fov = input_sys->camera->fov;
    fov -= (float)yoffset;
    CLAMP(input_sys->camera->fov, 1.0f, 90.0f);
    rpe_camera_set_fov(input_sys->camera, fov);

    if (input_sys->show_ui)
    {
        nk_instance_t* nk = input_sys->nk;
        nk->scroll.x += xoffset;
        nk->scroll.y += yoffset;
    }
}

void keyCallback(GLFWwindow* window, int key, int scan_code, int action, int mode)
{
    app_window_key_response(window, key, scan_code, action, mode);
}

void mouseButtonPressCallback(GLFWwindow* window, int button, int action, int mods)
{
    app_window_mouse_button_response(window, button, action, mods);
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    app_window_mouse_move_response(window, xpos, ypos);
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    app_window_scroll_response(window, xoffset, yoffset);
}
