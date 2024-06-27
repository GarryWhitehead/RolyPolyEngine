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
    bool success = glfwInit();
    if (!success)
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
    glfwSetCursorEnterCallback(new_win->glfw_window, cursorEnterCallback);

    // Create a new vulkan driver instance along with the window surface.
    uint32_t count = 0;
    const char** glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&count);

    int error_code = vkapi_driver_init(glfw_extensions, count, &app->driver);
    if (error_code != VKAPI_SUCCESS)
    {
        return APP_ERROR_NO_DEVICE;
    }

    // Create the window surface.
    VkSurfaceKHR surface;
    VkResult err = glfwCreateWindowSurface(
        app->driver.context.instance, new_win->glfw_window, VK_NULL_HANDLE, &new_win->vk_surface);
    if (err)
    {
        return APP_ERROR_NO_SURFACE;
    }

    // Create the abstract physical device object.
    error_code = vkapi_driver_create_device(&app->driver, new_win->vk_surface);
    if (error_code != VKAPI_SUCCESS)
    {
        return APP_ERROR_NO_DEVICE;
    }

    // create the engine (dependent on the glfw window for creating the device).
    app->engine = rpe_engine_create(&app->driver);

    return APP_SUCCESS;
}

void app_window_shutdown(app_window_t* win)
{
    glfwDestroyWindow(win->glfw_window);
    glfwTerminate();
}

void app_window_poll() { glfwPollEvents(); }

void keyCallback(GLFWwindow* window, int key, int scan_code, int action, int mode)
{
    app_window_t* inputSys = (app_window_t*)glfwGetWindowUserPointer(window);
    // inputSys->keyResponse(window, key, scan_code, action, mode);
}

void mouseButtonPressCallback(GLFWwindow* window, int button, int action, int mods)
{
    app_window_t* inputSys = (app_window_t*)glfwGetWindowUserPointer(window);
    // inputSys->mouseButtonResponse(window, button, action, mods);
}

void mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    app_window_t* inputSys = (app_window_t*)glfwGetWindowUserPointer(window);
    // inputSys->mouseMoveResponse(window, xpos, ypos);
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    app_window_t* inputSys = (app_window_t*)glfwGetWindowUserPointer(window);
    // inputSys->scrollResponse(window, xoffset, yoffset);
}

void cursorEnterCallback(GLFWwindow* window, int entered)
{
    app_window_t* inputSys = (app_window_t*)glfwGetWindowUserPointer(window);
    // inputSys->enterResponse(window, entered);
}
