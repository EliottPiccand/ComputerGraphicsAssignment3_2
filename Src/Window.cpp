#include "Window.h"

#include <cassert>
#include <cstddef>
#include <format>
#include <span>
#include <stdexcept>

#include <Lib/OpenGL.h>

#include "Events/EventQueue.h"
#include "Events/WindowResized.h"
#include "Utils/Profiling.h"

[[noreturn]] static void glfwErrorCallback(int code, const char *description)
{
    const std::string message = std::format("GLFW error ({}) : {}", code, description);
    throw std::runtime_error(message);
}

Window::Window() : is_full_screen_(false)
{
    glfwSetErrorCallback(glfwErrorCallback);
    glfwInit();

    handle_ = glfwCreateWindow(static_cast<int>(DEFAULT_WIDTH), static_cast<int>(DEFAULT_HEIGHT), DEFAULT_TITLE,
                               nullptr, nullptr);
    glfwMakeContextCurrent(handle_);

    glewInit();
    SetGpuProfilingContext;

    glfwSwapInterval(1); // vsync

    glfwSetFramebufferSizeCallback(handle_, [](GLFWwindow *window, int width, int height) {
        (void)window;
        EventQueue::post<event::WindowResized>(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    });
}

Window::~Window()
{
    glfwTerminate();
}

bool Window::shouldClose() const
{
    return glfwWindowShouldClose(handle_) == GLFW_TRUE;
}

void Window::endFrame() const
{
    ProfileScope;

    glfwSwapBuffers(handle_);
    CollectGpuProfilingEvents;
    glfwPollEvents();
}

void Window::captureMouse()
{
    glfwSetInputMode(handle_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Window::releaseMouse()
{
    glfwSetInputMode(handle_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void Window::setTitle(std::string title) const
{
    glfwSetWindowTitle(handle_, title.c_str());
}

std::pair<uint32_t, uint32_t> Window::getFramebufferSize() const
{
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(handle_, &width, &height);
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}

void Window::toggleFullScreen()
{
    is_full_screen_ = !is_full_screen_;

    if (is_full_screen_)
    {
        // get current monitor
        GLFWmonitor *current_monitor = nullptr;

        int current_window_x, current_window_y;
        glfwGetWindowPos(handle_, &current_window_x, &current_window_y);

        int count;
        GLFWmonitor **monitors_ptr = glfwGetMonitors(&count);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage-in-container"
        const std::span<GLFWmonitor *> monitors(monitors_ptr, static_cast<size_t>(count));
#pragma clang diagnostic pop

        for (const auto &monitor : monitors)
        {
            int monitor_x, monitory, width, height;
            glfwGetMonitorWorkarea(monitor, &monitor_x, &monitory, &width, &height);

            if ((monitor_x <= current_window_x && current_window_x < monitor_x + width) &&
                (monitory <= current_window_y && current_window_y < monitory + height))
            {
                current_monitor = monitor;
                break;
            }
        }

        assert(current_monitor != nullptr && "failed to retrive current monitor");

        // save current state
        int width, height;
        glfwGetWindowSize(handle_, &width, &height);

        non_full_screen_position_x_ = current_window_x;
        non_full_screen_position_y_ = current_window_y;
        non_full_screen_width_ = width;
        non_full_screen_height_ = height;

        // set fullscreen
        const GLFWvidmode *videoMode = glfwGetVideoMode(current_monitor);
        assert(videoMode != nullptr && "failed to retrieve current video mode");

        glfwSetWindowMonitor(handle_, current_monitor, 0, 0, videoMode->width, videoMode->height,
                             videoMode->refreshRate);
    }
    else
    {
        glfwSetWindowMonitor(handle_, nullptr, non_full_screen_position_x_, non_full_screen_position_x_,
                             non_full_screen_width_, non_full_screen_height_, GLFW_DONT_CARE);
    }
}

void Window::close()
{
    glfwSetWindowShouldClose(handle_, GLFW_TRUE);
}
