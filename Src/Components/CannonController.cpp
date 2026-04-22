#include "Components/CannonController.h"

#include <cmath>

#include <Lib/OpenGL.h>
#include <Lib/glfw.h>

#include "Components/Collider.h"
#include "Events/EventQueue.h"
#include "Events/Fire.h"
#include "GameObject.h" // IWYU pragma: keep
#include "Physics.h"
#include "Singleton.h"
#include "Utils/Color.h"
#include "Utils/Constants.h"
#include "Utils/Log.h"
#include "Utils/Math.h"

using namespace component;

CannonController::CannonController(std::weak_ptr<Transform> cannon_barrel_transform,
                                   std::weak_ptr<Transform> target_transform)
    : barrel_transform_(cannon_barrel_transform), target_transform_(target_transform), fired_(false), aiming_(false),
      recoil_(0.0f)
{
}

void CannonController::initialize()
{
    GET_COMPONENT(Transform, transform_, CannonController);

    std::shared_ptr<Collider> shooter_collider;
    GET_COMPONENT(Collider, shooter_collider, CannonController);
    shooter_id_ = shooter_collider->getOwner()->getId();
}

glm::vec3 CannonController::getShootingInitialVelocity(const glm::vec3 &target) const
{
    /// c.f. Ballistic.pdf
    const auto position = glm::vec3(barrel_transform_.lock()->resolve()[3]);

    const auto delta = target - position;

    const float a = GRAVITY * GRAVITY / 4.0f;
    const float b = glm::dot(delta, UP) * GRAVITY + INITIAL_CANNON_BALL_VELOCITY * INITIAL_CANNON_BALL_VELOCITY;
    const float c = glm::dot(delta, delta);

    const float disc = b * b - 4.0f * a * c;
    if (disc < 0.0f)
    {
        LOG_WARNING("target out of range");
        return {};
    }

    // const float T = (b + std::sqrtf(disc)) / (2.0f * a); // T_plus
    const float T = (b - std::sqrtf(disc)) / (2.0f * a); // T_minus
    const float t = std::sqrtf(T);
    return delta / t + 0.5f * GRAVITY * UP * t;
}

void CannonController::updateTarget(float delta_time)
{
    (void)delta_time;
}

void CannonController::update(float delta_time)
{
    constexpr const float RECOIL_AMPLITUDE = 0.3f; // m
    constexpr const float RECOIL_DECAY_INTENSITY = 0.9f;

    updateTarget(delta_time);

    target_transform_.lock()->setPosition(target_);

    auto transform = transform_.lock();
    auto barrel_transform = barrel_transform_.lock();

    auto resolved_transform = transform->resolve();
    const auto position = glm::vec3(resolved_transform[3]);

    // Cannon stand
    const auto planar_position = position - glm::dot(position, UP) * UP;
    const auto target_delta = target_ - planar_position;
    const auto target_direction = glm::normalize(target_delta);

    const auto cos_target_angle = glm::dot(NORTH, target_direction);
    const auto sin_target_angle = glm::dot(glm::cross(NORTH, target_direction), UP);
    const auto target_angle = std::atan2(sin_target_angle, cos_target_angle);

    const auto current_rotation_matrix = glm::mat3(resolved_transform);
    const auto current_rotation = glm::quat_cast(current_rotation_matrix);
    const auto current_angle = angleAroundAxis(current_rotation, UP);

    transform->rotate(target_angle - current_angle, UP);

    // Cannon barrel
    if (recoil_ > delta_time * RECOIL_DECAY_INTENSITY)
    {
        recoil_ -= delta_time * RECOIL_DECAY_INTENSITY;
    }
    else
    {
        recoil_ = 0.0f;
    }

    const auto barrel_parent_opt = barrel_transform->getOwner()->getParent();
    const auto barrel_parent_transform_opt = barrel_parent_opt.value()->getComponent<Transform>();
    const auto barrel_parent_rot = glm::quat_cast(glm::mat3(barrel_parent_transform_opt.value()->resolve()));
    const auto local_direction = glm::inverse(barrel_parent_rot) * cannon_ball_initial_velocity_;

    const auto direction = glm::normalize(local_direction);
    barrel_transform->pointToward(direction);
    barrel_transform->setPosition(-direction * recoil_);

    if (fired_)
    {
        EventQueue::post<event::Fire>(glm::vec3(barrel_transform->resolve()[3]), cannon_ball_initial_velocity_,
                                      shooter_id_);

        aiming_ = false;
        fired_ = false;
        recoil_ = RECOIL_AMPLITUDE;
    }
}

bool CannonController::render() const
{
    if (Singleton::debug)
    {
        PUSH_CLEAR_STATE();

        Singleton::active_camera.lock()->bind();

        const auto position = glm::vec3(barrel_transform_.lock()->resolve()[3]);
        const auto trajectory = Physics::simulateCannonballTrajectory(position, cannon_ball_initial_velocity_);

        constexpr const GLfloat material[] = {_v4(color::BLUE)};
        glMaterialfv(GL_FRONT, GL_AMBIENT, material);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, material);

        glLineWidth(3.0f);

        glBegin(GL_LINE_STRIP);
        for (const auto &trajectory_position : trajectory)
        {
            glVertex3f(_v3(trajectory_position));
        }
        glEnd();

        POP_CLEAR_STATE();
    }
    return false;
}
