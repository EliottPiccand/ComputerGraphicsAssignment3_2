#include "Input.h"

#include <cassert>
#include <cstddef>

constexpr const size_t MASK_OFFSET = sizeof(unsigned int) * 8 / 2;
constexpr const unsigned int MASK = (1 << MASK_OFFSET) - 1;

void Input::initialize(const Window &window)
{
    window_handle_ = window.handle_;

    // disable mouse acceleration
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window.handle_, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
}

void Input::bindKey(Action action, unsigned int key)
{
    key += 1;
    assert((key & MASK) == key && "`key` must be a `GLFW_KEY_...` macro");

    binds_[action] = key;
    states_[action] = State::HeldReleased;
}

void Input::bindMouseButton(Action action, unsigned int mouseButton)
{
    mouseButton += 1;
    assert((mouseButton & MASK) == mouseButton && "`mouseButton` must be a `GLFW_MOUSE_BUTTON_...` macro");

    binds_[action] = mouseButton << MASK_OFFSET;
    states_[action] = State::HeldReleased;
}

void Input::update()
{
    assert(window_handle_ != nullptr && "calling Input::update before initializing");

    // mouse motion
    last_mouse_position_ = mouse_position_;
    mouse_position_ = fetchMousePosition();
    mouse_delta_ = last_mouse_position_ - mouse_position_;

    // keys
    for (const auto &[action, key] : binds_)
    {
        const auto glfw_state = ((key & (MASK << MASK_OFFSET)) == 0)
                                    ? glfwGetKey(window_handle_, static_cast<int>(key - 1))
                                    : glfwGetMouseButton(window_handle_, (key >> MASK_OFFSET) - 1);

        if (glfw_state == GLFW_PRESS)
        {
            switch (states_[action])
            {
            case State::JustPressed:
            case State::HeldPressed:
                states_[action] = State::HeldPressed;
                break;
            case State::JustReleased:
            case State::HeldReleased:
                states_[action] = State::JustPressed;
                break;
            }
        }
        else // glfw_state == GLFW_RELEASE
        {
            switch (states_[action])
            {
            case State::JustPressed:
            case State::HeldPressed:
                states_[action] = State::JustReleased;
                break;
            case State::JustReleased:
            case State::HeldReleased:
                states_[action] = State::HeldReleased;
                break;
            }
        }
    }
}

Input::State Input::getState(Action action)
{
    return states_.at(action);
}

bool Input::isPressed(Action action)
{
    return states_.at(action) == State::JustPressed || states_.at(action) == State::HeldPressed;
}

glm::vec2 Input::fetchMousePosition()
{
    assert(window_handle_ != nullptr && "calling Input::fetchMousePosition before initializing");

    double x_pos, y_pos;
    glfwGetCursorPos(window_handle_, &x_pos, &y_pos);

    int window_width = 0;
    int window_height = 0;
    glfwGetWindowSize(window_handle_, &window_width, &window_height);

    int framebuffer_width = 0;
    int framebuffer_height = 0;
    glfwGetFramebufferSize(window_handle_, &framebuffer_width, &framebuffer_height);

    const float scale_x =
        window_width > 0 ? static_cast<float>(framebuffer_width) / static_cast<float>(window_width) : 1.0f;
    const float scale_y =
        window_height > 0 ? static_cast<float>(framebuffer_height) / static_cast<float>(window_height) : 1.0f;

    return {static_cast<float>(x_pos) * scale_x, static_cast<float>(y_pos) * scale_y};
}

glm::vec2 Input::getMousePosition()
{
    return mouse_position_;
}

glm::vec2 Input::getMouseDelta()
{
    return mouse_delta_;
}
