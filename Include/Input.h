#pragma once

#include <unordered_map>

#include <Lib/glfw.h>
#include <Lib/glm.h>

#include "Window.h"

class Input
{
  public:
    enum class Action
    {
        SpeedUp,
        SpeedDown,
        TurnLeft,
        TurnRight,
        AimAndFire,
        CancelFire,
        ToggleFullScreen,
        UIClick,
        FreeViewForward,
        FreeViewLeft,
        FreeViewBackward,
        FreeViewRight,
        FreeViewUp,
        FreeViewDown,
        ToggleFreeView,
        CycleRenderingStyles,
        ToggleDebugMode,
        DebugMoveTargetNorth,
        DebugMoveTargetEast,
        DebugMoveTargetSouth,
        DebugMoveTargetWest,
        DebugAimAndFire,
        CycleCameras,
        PauseTime,
        RestartGame,
        QuitGame,
    };

    enum class State
    {
        JustPressed,
        HeldPressed,
        JustReleased,
        HeldReleased,
    };

    static void initialize(const Window &window);

    static void bindKey(Action action, unsigned int key);
    static void bindMouseButton(Action action, unsigned int mouseButton);
    static void update();

    [[nodiscard]] static State getState(Action action);
    [[nodiscard]] static bool isPressed(Action action);
    [[nodiscard]] static glm::vec2 getMousePosition();
    [[nodiscard]] static glm::vec2 getMouseDelta();

  private:
    static inline GLFWwindow *window_handle_ = nullptr;
    static inline std::unordered_map<Action, unsigned int> binds_;
    static inline std::unordered_map<Action, State> states_;

    static inline glm::vec2 last_mouse_position_;
    static inline glm::vec2 mouse_position_;
    static inline glm::vec2 mouse_delta_;

    [[nodiscard]] static glm::vec2 fetchMousePosition();
};
