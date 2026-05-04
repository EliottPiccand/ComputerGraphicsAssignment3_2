#include "Components/CannonAIController.h"

#include <Lib/OpenGL.h>

#include "Utils/Constants.h"
#include "Utils/Profiling.h"
#include "Utils/Random.h"
#include "Utils/Time.h"

using namespace component;

CannonAIController::CannonAIController(std::weak_ptr<Transform> cannon_barrel_transform,
                                       std::weak_ptr<Transform> target_transform,
                                       std::weak_ptr<Transform> target_target_transform)
    : CannonController(cannon_barrel_transform, target_transform), last_fire_tick_(Time::now()),
      target_target_transform_(target_target_transform)
{
}

void CannonAIController::initialize()
{
    ProfileScope;

    CannonController::initialize();
    pickTargetTarget();
    target_ = target_target_;
    pickTargetTarget();
}

void CannonAIController::pickTargetTarget()
{
    ProfileScope;

    constexpr const float MARGIN = 20.0f;
    constexpr const float MIN_DISTANCE = 50.0f;

    do
    {
        target_target_ = Random::random(-WORLD_WIDTH / 2.0f + MARGIN, WORLD_WIDTH / 2.0f - MARGIN) * EAST +
                         Random::random(-WORLD_WIDTH / 2.0f + MARGIN, WORLD_WIDTH / 2.0f - MARGIN) * NORTH;
    } while (glm::length(target_target_ - target_) < MIN_DISTANCE);

    target_target_transform_.lock()->setPosition(target_target_);
}

void CannonAIController::updateTarget()
{
    ProfileScope;

    if (Time::paused)
        return;

    constexpr const float TARGET_REACH_RADIUS = 1.0f;  // m
    constexpr const float TARGET_MOVING_SPEED = 20.0f; // m/s
    constexpr const Duration MIN_FIRE_INTERVAL = Duration::milliseconds(200);
    constexpr const float FIRE_PROBABILITY_ON_EACH_TICK = 0.1f;

    if (glm::length(target_target_ - target_) < TARGET_REACH_RADIUS)
    {
        pickTargetTarget();
    }

    target_ += glm::normalize(target_target_ - target_) * TARGET_MOVING_SPEED * Time::getDeltaTime();
    cannon_ball_initial_velocity_ = getShootingInitialVelocity(target_);

    if (Time::now() >= last_fire_tick_ + MIN_FIRE_INTERVAL)
    {
        last_fire_tick_ = Time::now();

        if (Random::random(0.0f, 1.0f) < FIRE_PROBABILITY_ON_EACH_TICK)
        {
            fired_ = true;
        }
    }
}
