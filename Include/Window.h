#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include <Lib/glfw.h>

class Input;

class Window
{
  public:
    static constexpr const char *DEFAULT_TITLE = "Computer Graphics Assignment #3";
    static constexpr const uint16_t DEFAULT_WIDTH = 1280;
    static constexpr const uint16_t DEFAULT_HEIGHT = 720;

    Window();
    ~Window();

    [[nodiscard]] bool shouldClose() const;
    void endFrame() const;

    void setTitle(std::string title) const;
    void toggleFullScreen();
    void captureMouse();
    void releaseMouse();

    [[nodiscard]] std::pair<uint32_t, uint32_t> getFramebufferSize() const;

    void close();

  private:
    friend Input;

    GLFWwindow *handle_;

    int non_full_screen_position_x_;
    int non_full_screen_position_y_;
    int non_full_screen_width_;
    int non_full_screen_height_;
    bool is_full_screen_;
};
