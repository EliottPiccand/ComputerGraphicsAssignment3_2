#include "Components/ShipAIController.h"

#include <algorithm>

#include <Lib/OpenGL.h>

#include "Singleton.h"
#include "Utils/Color.h"
#include "Utils/Constants.h"
#include "Utils/Math.h"
#include "Utils/Random.h"

using namespace component;

ShipAIController::ShipAIController(std::weak_ptr<Transform> target_transform)
    : ShipController(), target_transform_(target_transform)
{
    speed_state_ = SpeedState::Forward;
}

void ShipAIController::initialize()
{
    ShipController::initialize();
    pickTarget();
}

void ShipAIController::pickTarget()
{
    constexpr const float MARGIN = 20.0f;
    constexpr const float MIN_DISTANCE = 50.0f;

    const auto position_3d = glm::vec3(transform_.lock()->resolve()[3]);
    const auto position = glm::vec2(glm::dot(position_3d, EAST), glm::dot(position_3d, NORTH));

    do
    {
        target_ = {
            Random::random(-WORLD_WIDTH / 2.0f + MARGIN, WORLD_WIDTH / 2.0f - MARGIN),
            Random::random(-WORLD_WIDTH / 2.0f + MARGIN, WORLD_WIDTH / 2.0f - MARGIN),
        };
    } while (glm::length(position - target_) < MIN_DISTANCE);

    target_transform_.lock()->setPosition(target_.x * EAST + target_.y * NORTH);
}

void ShipAIController::updateStates()
{
    constexpr const float TARGET_REACH_RADIUS = 5.0f;
    constexpr const float TURN_DEAD_ZONE = SHIP_TURN_STEP * 0.5f;

    const auto transform = transform_.lock();
    const auto position_3d = glm::vec3(transform->resolve()[3]);
    const auto position = glm::vec2(glm::dot(position_3d, EAST), glm::dot(position_3d, NORTH));

    if (glm::length(position - target_) < TARGET_REACH_RADIUS)
    {
        pickTarget();
    }

    auto forward_3d = getForwardVector(transform->getRotation());
    forward_3d -= UP * glm::dot(forward_3d, UP);
    if (glm::length(forward_3d) <= EPSILON)
    {
        forward_3d = NORTH;
    }
    else
    {
        forward_3d = glm::normalize(forward_3d);
    }

    const auto to_target = target_ - position;
    if (glm::length(to_target) <= EPSILON)
    {
        turn_state_ = TurnState::None;
        return;
    }

    const auto target_direction_3d = glm::normalize(to_target.x * EAST + to_target.y * NORTH);
    const float sin_angle = glm::dot(glm::cross(forward_3d, target_direction_3d), UP);
    const float cos_angle = glm::dot(forward_3d, target_direction_3d);
    const float heading_error = std::atan2(sin_angle, cos_angle);

    turn_speed_ = std::min(std::abs(heading_error), SHIP_TURN_STEP);

    if (std::abs(heading_error) <= TURN_DEAD_ZONE)
    {
        turn_state_ = TurnState::None;
    }
    else
    {
        turn_state_ = heading_error > 0.0f ? TurnState::Left : TurnState::Right;
    }
}

bool ShipAIController::render() const
{
    if (Singleton::debug)
    {
        PUSH_CLEAR_STATE();
        Singleton::active_camera.lock()->bind();

        const auto position = glm::vec3(transform_.lock()->resolve()[3]);
        const auto target_3d = target_.x * EAST + target_.y * NORTH;

        glMaterialfv(GL_FRONT, GL_AMBIENT, color::MATERIAL_GREEN);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, color::MATERIAL_GREEN);

        glLineWidth(5.0f);

        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, 0x00FF);

        glBegin(GL_LINES);
            glVertex3f(_v3(position));
            glVertex3f(_v3(target_3d));
        glEnd();

        glDisable(GL_LINE_STIPPLE);
        
        POP_CLEAR_STATE();
    }
    
    return false;
}
