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

#include "window.h"

#include "app.h"
#include "rpe/engine.h"
#include "vulkan-api/driver.h"
#include "vulkan-api/error_codes.h"

int app_window_init(
    rpe_app_t* app, const char* title, uint32_t width, uint32_t height, app_window_t* new_win)
{
    memset(new_win, 0, sizeof(app_window_t));

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
    app->engine = rpe_engine_create(app->driver);

    new_win->cam_view = rpe_camera_view_init(app->engine);
    new_win->camera = rpe_camera_init(app->driver);

    uint32_t g_width, g_height;
    glfwGetWindowSize(app->window.glfw_window, (int*)&g_width, (int*)&g_height);
    rpe_camera_set_projection(
        &app->window.camera,
        app->camera_fov,
        (float)g_width / (float)g_height,
        app->camera_near,
        app->camera_far,
        RPE_PROJECTION_TYPE_PERSPECTIVE);

    // Create a scene for our application.
    app->scene = rpe_scene_create(app->engine);
    rpe_scene_set_current_camera(app->scene, &new_win->camera);
    rpe_engine_set_current_scene(app->engine, app->scene);

    return APP_SUCCESS;
}

void app_window_shutdown(app_window_t* win, rpe_app_t* app)
{
    assert(win);
    assert(app);

    vkapi_driver_shutdown(app->driver, win->vk_surface);
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
        }
        else if (action == GLFW_RELEASE)
        {
            rpe_camera_view_mouse_button_up(&input_sys->cam_view);
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
    input_sys->camera.fov -= (float)yoffset;
    CLAMP(input_sys->camera.fov, 1.0f, 90.0f);
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
