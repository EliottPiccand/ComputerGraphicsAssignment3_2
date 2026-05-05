#include "Window.h"

#include <cassert>
#include <cstddef>
#include <format>
#include <span>
#include <stdexcept>

#include <Lib/OpenGL.h>

#include "Events/EventQueue.h"
#include "Events/WindowResized.h"
#include "Utils/Log.h"
#include "Utils/Profiling.h"

namespace
{
const char *glDebugSourceName(GLenum source)
{
    switch (source)
    {
    case GL_DEBUG_SOURCE_API:
        return "API";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        return "Window System";
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        return "Shader Compiler";
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        return "Third Party";
    case GL_DEBUG_SOURCE_APPLICATION:
        return "Application";
    case GL_DEBUG_SOURCE_OTHER:
        return "Other";
    default:
        return "Unknown";
    }
}

const char *glDebugTypeName(GLenum type)
{
    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:
        return "Error";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        return "Deprecated Behavior";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        return "Undefined Behavior";
    case GL_DEBUG_TYPE_PORTABILITY:
        return "Portability";
    case GL_DEBUG_TYPE_PERFORMANCE:
        return "Performance";
    case GL_DEBUG_TYPE_OTHER:
        return "Other";
    case GL_DEBUG_TYPE_MARKER:
        return "Marker";
    case GL_DEBUG_TYPE_PUSH_GROUP:
        return "Push Group";
    case GL_DEBUG_TYPE_POP_GROUP:
        return "Pop Group";
    default:
        return "Unknown";
    }
}

const char *glDebugSeverityName(GLenum severity)
{
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:
        return "High";
    case GL_DEBUG_SEVERITY_MEDIUM:
        return "Medium";
    case GL_DEBUG_SEVERITY_LOW:
        return "Low";
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        return "Notification";
    default:
        return "Unknown";
    }
}
} // namespace

[[noreturn]] static void glfwErrorCallback(int code, const char *description)
{
    const std::string message = std::format("GLFW error ({}) : {}", code, description);
    throw std::runtime_error(message);
}

Window::Window() : is_full_screen_(false)
{
    glfwSetErrorCallback(glfwErrorCallback);
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    handle_ = glfwCreateWindow(static_cast<int>(DEFAULT_WIDTH), static_cast<int>(DEFAULT_HEIGHT), DEFAULT_TITLE,
                               nullptr, nullptr);
    glfwMakeContextCurrent(handle_);

    glewInit();
    SetGpuProfilingContext;

#if defined(OE_DEBUG)
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

    int major = 0, minor = 0;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);

    LOG_DEBUG("OpenGL Context: {}.{}", major, minor);

    int profile_mask = 0;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile_mask);
    if (profile_mask == GL_CONTEXT_CORE_PROFILE_BIT)
    {
        LOG_DEBUG("Profile: CORE");
    }
    else if (profile_mask == GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)
    {
        LOG_WARNING("Profile: COMPATIBILITY");
    }
    else
    {
        LOG_WARNING("Profile: UNKNOWN");
    }

    glDebugMessageCallback(
        [](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message,
           const void *userParam) {
            (void)id;
            (void)length;
            (void)userParam;

            const char *text = message != nullptr ? message : "no description";

            if (type == GL_DEBUG_TYPE_ERROR || type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR)
            {
                LOG_ERROR("OpenGL {} {} [{} / id {}]: {}", glDebugSourceName(source), glDebugTypeName(type),
                          glDebugSeverityName(severity), id, text);

#if defined(__GNUC__) || defined(__clang__)
                __builtin_trap();
#endif
            }
            else
            {
                LOG_WARNING("OpenGL {} {} [{} / id {}]: {}", glDebugSourceName(source), glDebugTypeName(type),
                            glDebugSeverityName(severity), id, text);
            }
        },
        nullptr);
#endif

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
