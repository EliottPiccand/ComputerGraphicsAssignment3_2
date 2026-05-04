#include "Components/CannonPlayerController.h"

#include <Lib/OpenGL.h>
#include <Lib/glfw.h>
#include <Lib/glm.h>

#include "Input.h"
#include "Singleton.h"
#include "Utils/Constants.h"
#include "Utils/Log.h"
#include "Utils/Math.h"
#include "Utils/Profiling.h"
#include "Utils/View.h"

using namespace component;

CannonPlayerController::CannonPlayerController(std::weak_ptr<Transform> cannon_barrel_transform,
                                               std::weak_ptr<Transform> target_transform,
                                               std::weak_ptr<Camera3D> camera)
    : CannonController(cannon_barrel_transform, target_transform), camera_(camera)
{
    Input::bindMouseButton(Input::Action::AimAndFire, GLFW_MOUSE_BUTTON_LEFT);
    Input::bindMouseButton(Input::Action::CancelFire, GLFW_MOUSE_BUTTON_RIGHT);
}

void CannonPlayerController::updateTarget()
{
    ProfileScope;

    constexpr const float DEBUG_TARGET_SPEED = 15.0f; // m/s

    auto transform = transform_.lock();
    auto barrel_transform = barrel_transform_.lock();

    if (Input::getState(Singleton::view != View::Top ? Input::Action::DebugAimAndFire : Input::Action::AimAndFire) ==
        Input::State::JustPressed)
    {
        LOG_TRACE("aiming");
        aiming_ = true;
    }

    if (Input::getState(Input::Action::CancelFire) == Input::State::JustReleased)
    {
        aiming_ = false;
        LOG_TRACE("fire canceled");
    }

    glm::vec3 target;

    glm::vec3 target_motion{};
    if (Input::isPressed(Input::Action::DebugMoveTargetNorth))
    {
        target_motion += NORTH;
    }
    if (Input::isPressed(Input::Action::DebugMoveTargetEast))
    {
        target_motion += EAST;
    }
    if (Input::isPressed(Input::Action::DebugMoveTargetSouth))
    {
        target_motion += SOUTH;
    }
    if (Input::isPressed(Input::Action::DebugMoveTargetWest))
    {
        target_motion += WEST;
    }

    if (aiming_ && Singleton::view == View::Top)
    {
        target = Singleton::active_camera.lock()->screenToWorld(Input::getMousePosition());
    }
    else
    {
        target = glm::vec3(target_transform_.lock()->resolve()[3]);

        if (glm::length(target_motion) > EPSILON)
        {
            target += glm::normalize(target_motion) * DEBUG_TARGET_SPEED * Time::getDeltaTime();
        }
    }

    const auto target_along_north = glm::dot(target, NORTH);
    const auto target_along_east = glm::dot(target, EAST);
    const bool target_in_bounds =
        !(target_along_north < -WORLD_WIDTH / 2.0f || WORLD_WIDTH / 2.0f < target_along_north ||
          target_along_east < -WORLD_WIDTH / 2.0f || WORLD_WIDTH / 2.0f < target_along_east);
    if (!target_in_bounds)
    {
        aiming_ = false;
    }

    if (target_in_bounds)
    {
        target -= glm::dot(target, UP) * UP;

        cannon_ball_initial_velocity_ = getShootingInitialVelocity(target);
        target_ = target;

        // Camera
        const auto planar_velocity = cannon_ball_initial_velocity_ - UP * glm::dot(UP, cannon_ball_initial_velocity_);
        if (glm::length(planar_velocity) > EPSILON)
        {
            camera_.lock()->lookToward(glm::normalize(planar_velocity));
        }
    }

    if (Input::getState(Singleton::view != View::Top ? Input::Action::DebugAimAndFire : Input::Action::AimAndFire) ==
        Input::State::JustReleased)
    {
        if (aiming_)
        {
            fired_ = true;
        }
    }
}
