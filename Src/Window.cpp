#include "Window.h"

#include <cassert>
#include <cstddef>
#include <format>
#include <span>
#include <stdexcept>

#include <Lib/OpenGL.h>

#include "Events/EventQueue.h"
#include "Events/WindowResized.h"
#include "Resources/Texture.h"
#include "Utils/Color.h"
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

Window::Window() : is_full_screen_(false), width_(DEFAULT_WIDTH), height_(DEFAULT_HEIGHT)
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
    createFrameBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT);

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

    EventQueue::registerCallback<event::WindowResized>([&](const event::WindowResized &event) {
        deleteColorDepthNormalsTextures();
        createColorDepthNormalsTextures(event.width, event.height);

        width_ = event.width;
        height_ = event.height;

        glViewport(0, 0, static_cast<GLsizei>(event.width), static_cast<GLsizei>(event.height));
    });
}

Window::~Window()
{
    deleteFrameBuffer();

    glfwTerminate();
}

bool Window::shouldClose() const
{
    return glfwWindowShouldClose(handle_) == GLFW_TRUE;
}

void Window::startRendering() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_);

    glClearColor(color::SKY.r, color::SKY.g, color::SKY.b, color::SKY.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::bindFrameBuffer() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_);
}

void Window::unbindFrameBuffer() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Window::mapFrameBuffer(const std::vector<std::weak_ptr<resource::Shader>> &shaders) const
{
    glCopyImageSubData(color_texture_, GL_TEXTURE_2D, 0, 0, 0, 0, color_texture_snapshot_, GL_TEXTURE_2D, 0, 0, 0, 0,
                       static_cast<GLsizei>(width_), static_cast<GLsizei>(height_), 1);
    glCopyImageSubData(depth_texture_, GL_TEXTURE_2D, 0, 0, 0, 0, depth_texture_snapshot_, GL_TEXTURE_2D, 0, 0, 0, 0,
                       static_cast<GLsizei>(width_), static_cast<GLsizei>(height_), 1);
    glCopyImageSubData(normals_texture_, GL_TEXTURE_2D, 0, 0, 0, 0, normals_texture_snapshot_, GL_TEXTURE_2D, 0, 0, 0,
                       0, static_cast<GLsizei>(width_), static_cast<GLsizei>(height_), 1);

    for (auto weak_shader : shaders)
    {
        auto shader = weak_shader.lock();
        shader->bind();
        
        const auto color_location = shader->getUniformLocation("u_HDRMap");
        glBindTextureUnit(resource::Texture::COLOR_SLOT, color_texture_snapshot_);
        glUniform1i(color_location, static_cast<GLint>(resource::Texture::COLOR_SLOT));

        const auto depth_location = shader->getUniformLocation("u_DepthMap");
        glBindTextureUnit(resource::Texture::DEPTH_SLOT, depth_texture_snapshot_);
        glUniform1i(depth_location, static_cast<GLint>(resource::Texture::DEPTH_SLOT));

        const auto normals_location = shader->getUniformLocation("u_NormalMap");
        glBindTextureUnit(resource::Texture::NORMALS_SLOT, normals_texture_snapshot_);
        glUniform1i(normals_location, static_cast<GLint>(resource::Texture::NORMALS_SLOT));
    }
}

void Window::endFrame() const
{
    ProfileScope;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, frame_buffer_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    glBlitFramebuffer(
        0, 0, static_cast<GLint>(width_), static_cast<GLint>(height_),
        0, 0, static_cast<GLint>(width_), static_cast<GLint>(height_),
        GL_COLOR_BUFFER_BIT,
        GL_NEAREST
    );
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

void Window::createColorDepthNormalsTextures(uint32_t width, uint32_t height)
{
    glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer_);

    constexpr const GLenum COLOR_ATTACHMENTS[] = {
        GL_COLOR_ATTACHMENT0, // color
        GL_COLOR_ATTACHMENT1, // normals
    };

    glCreateTextures(GL_TEXTURE_2D, 1, &color_texture_);
    glTextureStorage2D(color_texture_, 1, GL_RGBA16F, static_cast<int>(width), static_cast<int>(height));
    glTextureParameteri(color_texture_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(color_texture_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(color_texture_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(color_texture_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glNamedFramebufferTexture(frame_buffer_, COLOR_ATTACHMENTS[0], color_texture_, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &color_texture_snapshot_);
    glTextureStorage2D(color_texture_snapshot_, 1, GL_RGBA16F, static_cast<int>(width), static_cast<int>(height));
    glTextureParameteri(color_texture_snapshot_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(color_texture_snapshot_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(color_texture_snapshot_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(color_texture_snapshot_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glCreateTextures(GL_TEXTURE_2D, 1, &depth_texture_);
    glTextureStorage2D(depth_texture_, 1, GL_DEPTH_COMPONENT32F, static_cast<int>(width), static_cast<int>(height));
    glTextureParameteri(depth_texture_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(depth_texture_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(depth_texture_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(depth_texture_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glNamedFramebufferTexture(frame_buffer_, GL_DEPTH_ATTACHMENT, depth_texture_, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &depth_texture_snapshot_);
    glTextureStorage2D(depth_texture_snapshot_, 1, GL_DEPTH_COMPONENT32F, static_cast<int>(width), static_cast<int>(height));
    glTextureParameteri(depth_texture_snapshot_, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(depth_texture_snapshot_, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(depth_texture_snapshot_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(depth_texture_snapshot_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glCreateTextures(GL_TEXTURE_2D, 1, &normals_texture_);
    glTextureStorage2D(normals_texture_, 1, GL_RGBA16F, static_cast<int>(width), static_cast<int>(height));
    glTextureParameteri(normals_texture_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(normals_texture_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(normals_texture_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(normals_texture_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glNamedFramebufferTexture(frame_buffer_, COLOR_ATTACHMENTS[1], normals_texture_, 0);

    glCreateTextures(GL_TEXTURE_2D, 1, &normals_texture_snapshot_);
    glTextureStorage2D(normals_texture_snapshot_, 1, GL_RGBA16F, static_cast<int>(width), static_cast<int>(height));
    glTextureParameteri(normals_texture_snapshot_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(normals_texture_snapshot_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(normals_texture_snapshot_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(normals_texture_snapshot_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glNamedFramebufferDrawBuffers(frame_buffer_, 2, COLOR_ATTACHMENTS);

    if (glCheckNamedFramebufferStatus(frame_buffer_, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        LOG_ERROR("failed to update the frame buffer");
        throw std::runtime_error("failed to update the frame buffer");
    }
}

void Window::deleteColorDepthNormalsTextures()
{
    glDeleteTextures(1, &color_texture_);
    glDeleteTextures(1, &depth_texture_);
    glDeleteTextures(1, &normals_texture_);
    glDeleteTextures(1, &color_texture_snapshot_);
    glDeleteTextures(1, &depth_texture_snapshot_);
    glDeleteTextures(1, &normals_texture_snapshot_);
}

void Window::createFrameBuffer(uint32_t width, uint32_t height)
{
    glCreateFramebuffers(1, &frame_buffer_);

    createColorDepthNormalsTextures(width, height);
}

void Window::deleteFrameBuffer()
{
    deleteColorDepthNormalsTextures();
    glDeleteFramebuffers(1, &frame_buffer_);
}
