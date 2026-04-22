#include "Components/CannonAIController.h"

#include <Lib/OpenGL.h>
#include <chrono>

#include "Singleton.h"
#include "Utils/Color.h"
#include "Utils/Constants.h"
#include "Utils/Random.h"

using namespace component;

CannonAIController::CannonAIController(std::weak_ptr<Transform> cannon_barrel_transform,
                                       std::weak_ptr<Transform> target_transform,
                                       std::weak_ptr<Transform> target_target_transform)
    : CannonController(cannon_barrel_transform, target_transform), last_fire_tick_(now()),
      target_target_transform_(target_target_transform)
{
}

void CannonAIController::initialize()
{
    CannonController::initialize();
    pickTargetTarget();
    target_ = target_target_;
    pickTargetTarget();
}

void CannonAIController::pickTargetTarget()
{
    constexpr const float MARGIN = 20.0f;
    constexpr const float MIN_DISTANCE = 50.0f;

    do
    {
        target_target_ = Random::random(-WORLD_WIDTH / 2.0f + MARGIN, WORLD_WIDTH / 2.0f - MARGIN) * EAST +
                         Random::random(-WORLD_WIDTH / 2.0f + MARGIN, WORLD_WIDTH / 2.0f - MARGIN) * NORTH;
    } while (glm::length(target_target_ - target_) < MIN_DISTANCE);

    target_target_transform_.lock()->setPosition(target_target_);
}

void CannonAIController::updateTarget(float delta_time)
{
    if (Singleton::physics_paused)
        return;

    constexpr const float TARGET_REACH_RADIUS = 1.0f;  // m
    constexpr const float TARGET_MOVING_SPEED = 20.0f; // m/s
    constexpr const Duration MIN_FIRE_INTERVAL = std::chrono::milliseconds(200);
    constexpr const float FIRE_PROBABILITY_ON_EACH_TICK = 0.1f;

    if (glm::length(target_target_ - target_) < TARGET_REACH_RADIUS)
    {
        pickTargetTarget();
    }

    target_ += glm::normalize(target_target_ - target_) * TARGET_MOVING_SPEED * delta_time;
    cannon_ball_initial_velocity_ = getShootingInitialVelocity(target_);

    if (now() >= last_fire_tick_ + MIN_FIRE_INTERVAL)
    {
        last_fire_tick_ = now();

        if (Random::random(0.0f, 1.0f) < FIRE_PROBABILITY_ON_EACH_TICK)
        {
            fired_ = true;
        }
    }
}

bool CannonAIController::render() const
{
    if (Singleton::debug)
    {
        PUSH_CLEAR_STATE();
        Singleton::active_camera.lock()->bind();

        const auto target_position = glm::vec3(target_transform_.lock()->resolve()[3]);
        const auto target_target_position = glm::vec3(target_target_transform_.lock()->resolve()[3]);

        glMaterialfv(GL_FRONT, GL_AMBIENT, color::MATERIAL_GREEN);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, color::MATERIAL_GREEN);

        glLineWidth(5.0f);

        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, 0x00FF);

        glBegin(GL_LINES);
        glVertex3f(_v3(target_position));
        glVertex3f(_v3(target_target_position));
        glEnd();

        glDisable(GL_LINE_STIPPLE);

        POP_CLEAR_STATE();
    }

    return CannonController::render();
}
