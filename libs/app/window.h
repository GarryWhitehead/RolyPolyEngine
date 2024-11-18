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

#ifndef __APP_WINDOW_H__
#define __APP_WINDOW_H__

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdint.h>

// Forward declarations.
typedef struct Application rpe_app_t;

enum
{
    APP_SUCCESS,
    APP_ERROR_GLFW_NOT_INIT,
    APP_ERROR_NO_WINDOW,
    APP_ERROR_NO_SURFACE,
    APP_ERROR_NO_DEVICE
};

typedef struct AppWindow
{
    /// Current window dimensions in pixels.
    uint32_t width;
    uint32_t height;

    /// A GLFW window 
    GLFWwindow* glfw_window;
    /// A GLFW window object. Is NULL if fullscreen mode selected.
    GLFWmonitor* glfw_monitor;
    /// GFLW vsync monitor mode.
    const GLFWvidmode* glfw_vmode;
    /// A Vulkan window surface obtained from GLFW. Will be NULL of running in headless mode.
    VkSurfaceKHR vk_surface;
} app_window_t;

/**
 Initialise a new window.
 @param app A pointer to an initialised app object.
 @param title The title of the window.
 @param width The width of the window in pixels.
 @param height The height of the window in pixels.
 @param new_win A pointer to a window object which will be initialised.
 @returns a error code determining how successful the initialisation was.
 */
int app_window_init(
    rpe_app_t* app, const char* title, uint32_t width, uint32_t height, app_window_t* new_win);

/**
 Shutdown the resources for this window instance.
 @param win A pointer to the window instance to close.
 */
void app_window_shutdown(app_window_t* win);

/**
 Poll the window for input (from keyboard, mouse, gamepad, etc.).
 @note A window instance must have been initiated before calling this.
 */
void app_window_poll();

// Callbacks from GLFW
void keyCallback(GLFWwindow* window, int key, int scan_code, int action, int mode);

void mouseButtonPressCallback(GLFWwindow* window, int button, int action, int mods);

void mouseCallback(GLFWwindow* window, double xpos, double ypos);

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

void cursorEnterCallback(GLFWwindow* window, int entered);

#endif
