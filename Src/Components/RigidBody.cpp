#include "Components/RigidBody.h"

#include "Physics.h"
#include "Utils/Math.h"
#include "Utils/Profiling.h"

using namespace component;

RigidBody::RigidBody() : is_static_(true), has_collisions_(true)
{
}

RigidBody::RigidBody(float mass) : RigidBody(mass, glm::mat3(1.0f))
{
}

RigidBody::RigidBody(float mass, glm::mat3 inertia)
    : is_static_(false), has_collisions_(true), mass_(mass), inverse_mass_(1.0f / mass),
      inverse_inertia_(glm::inverse(inertia)), velocity_({}), angular_velocity_({}),
      orientation_(glm::quat(1.0f, 0.0f, 0.0f, 0.0f))
{
}

void RigidBody::addForce(Force force)
{
    forces_.push_back(force);
}

const glm::vec3 &RigidBody::getVelocity() const
{
    return velocity_;
}

const glm::vec3 &RigidBody::getPosition() const
{
    return position_;
}

void RigidBody::setVelocity(const glm::vec3 &velocity)
{
    velocity_ = velocity;
}

void RigidBody::setOrientation(const glm::quat &orientation)
{
    orientation_ = orientation;
}

void RigidBody::initialize()
{
    ProfileScope;

    GET_COMPONENT(Collider, collider_, RigidBody);

    reset();

    Physics::addRigidBody(std::dynamic_pointer_cast<RigidBody>(Component::shared_from_this()));
}

void RigidBody::reset()
{
    const auto transform = collider_.lock()->transform_.lock();
    position_ = glm::vec3(transform->resolve()[3]);
    orientation_ = transform->getRotation();
}

void RigidBody::updatePhysics(float delta_time)
{
    ProfileScope;

    glm::vec3 forces_sum = ZERO;
    glm::vec3 torques_sum = ZERO;
    for (const auto &force_callback : forces_)
    {
        const auto [force, application_point] =
            force_callback(velocity_, position_, angular_velocity_, orientation_, mass_);
        forces_sum += force;
        torques_sum += glm::cross(application_point, force);
    }
    forces_.clear();

    const glm::vec3 acceleration = inverse_mass_ * forces_sum;
    velocity_ += acceleration * delta_time;
    position_ += velocity_ * delta_time;

    const glm::vec3 angular_acceleration = inverse_inertia_ * torques_sum;
    angular_velocity_ += angular_acceleration * delta_time;
    orientation_ += 0.5f * glm::quat(0, angular_velocity_) * orientation_ * delta_time;
    orientation_ = glm::normalize(orientation_);
}
