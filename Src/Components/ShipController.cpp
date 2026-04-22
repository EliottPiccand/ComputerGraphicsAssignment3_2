#include "Components/ShipController.h"

#include <chrono>
#include <cstddef>
#include <optional>
#include <vector>

#include "GameObject.h" // IWYU pragma: keep
#include "ParticleSystem.h"
#include "Utils/Color.h"
#include "Utils/Constants.h"
#include "Utils/Math.h"
#include "Utils/Random.h"
#include "Utils/Time.h"

using namespace component;

ShipController::ShipController()
    : speed_state_(SpeedState::Stopped), turn_state_(TurnState::None), turn_speed_(SHIP_TURN_STEP)
{
}

void ShipController::initialize()
{
    GET_COMPONENT(Transform, transform_, ShipController);
    GET_COMPONENT(RigidBody, rigid_body_, ShipController);
}

void ShipController::update(float delta_time)
{
    (void)delta_time;

    constexpr const glm::vec3 SHIP_FRONT = MODEL_FORWARD * 10.0f;
    constexpr const glm::vec3 SHIP_BACK = MODEL_BACKWARD * 11.0f;

    constexpr const size_t FOAM_PARTICLE_COUNT = 50;
    constexpr const Color FOAM_PARTICLE_COLOR = color::WHITE;
    constexpr const float FOAM_PARTICLE_SPREAD = 2.0f; // m

    updateStates();

    const auto transform = transform_.lock();
    const auto rigid_body = rigid_body_.lock();

    if (turn_state_ == TurnState::Left)
    {
        transform->rotate(turn_speed_, UP);
        rigid_body->setOrientation(transform->getRotation());
    }
    if (turn_state_ == TurnState::Right)
    {
        transform->rotate(-turn_speed_, UP);
        rigid_body->setOrientation(transform->getRotation());
    }

    const auto &current_velocity = rigid_body->getVelocity();
    auto forward = getForwardVector(transform->getRotation());
    forward -= UP * glm::dot(forward, UP);
    if (glm::length(forward) <= EPSILON)
    {
        forward = NORTH;
    }
    else
    {
        forward = glm::normalize(forward);
    }

    std::optional<glm::vec3> particle_offset;
    switch (speed_state_)
    {
    case SpeedState::Forward:
        rigid_body->setVelocity(UP * glm::dot(current_velocity, UP) + forward * SHIP_SPEED);
        particle_offset = SHIP_BACK;
        break;
        case SpeedState::Stopped:
        rigid_body->setVelocity(UP * glm::dot(current_velocity, UP));
        particle_offset = std::nullopt;
        break;
        case SpeedState::Backward:
        rigid_body->setVelocity(UP * glm::dot(current_velocity, UP) - forward * SHIP_SPEED);
        particle_offset = SHIP_FRONT;
        break;
    }

    if (particle_offset.has_value())
    {
        const auto rotated_offset = transform->getRotation() * particle_offset.value();
        const auto offset = rotated_offset + rigid_body->getPosition();
        const auto instant_now = now();

        std::vector<Particle> particles(FOAM_PARTICLE_COUNT);
        for (auto &particle : particles)
        {
            particle.color = FOAM_PARTICLE_COLOR;
            particle.position = offset + Random::direction() * Random::random(0.0f, FOAM_PARTICLE_SPREAD);
            particle.velocity = ZERO;
            particle.subject_to_gravity = true;
            particle.lifetime_start = instant_now;
            particle.max_lifetime = std::chrono::milliseconds(Random::randint(500, 2500));
        }

        ParticleSystem::addParticles(particles);
    }
}

void ShipController::updateStates()
{
}
