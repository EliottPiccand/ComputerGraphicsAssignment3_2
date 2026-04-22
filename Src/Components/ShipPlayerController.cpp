#include "Components/ShipPlayerController.h"

#include <Lib/glfw.h>

#include "Input.h"
#include "Singleton.h"
#include "Utils/View.h"

using namespace component;

ShipPlayerController::ShipPlayerController() : ShipController()
{
    Input::bindKey(Input::Action::SpeedUp, GLFW_KEY_W);
    Input::bindKey(Input::Action::TurnLeft, GLFW_KEY_A);
    Input::bindKey(Input::Action::SpeedDown, GLFW_KEY_S);
    Input::bindKey(Input::Action::TurnRight, GLFW_KEY_D);
}

void ShipPlayerController::stop()
{
    speed_state_ = SpeedState::Stopped;
}

void ShipPlayerController::updateStates()
{
    if (Singleton::view != View::FreeCamera)
    {
        // Update speed state
        if (Input::getState(Input::Action::SpeedUp) == Input::State::JustReleased)
        {
            speed_state_ = speed_state_ == SpeedState::Backward ? SpeedState::Stopped : SpeedState::Forward;
        }
        if (Input::getState(Input::Action::SpeedDown) == Input::State::JustReleased)
        {
            speed_state_ = speed_state_ == SpeedState::Forward ? SpeedState::Stopped : SpeedState::Backward;
        }

        turn_state_ = TurnState::None;
        if (Input::getState(Input::Action::TurnLeft) == Input::State::JustReleased)
        {
            turn_state_ = TurnState::Left;
        }
        if (Input::getState(Input::Action::TurnRight) == Input::State::JustReleased)
        {
            turn_state_ = TurnState::Right;
        }
    }
}
