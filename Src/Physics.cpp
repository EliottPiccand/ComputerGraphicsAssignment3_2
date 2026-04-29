#include "Physics.h"

#include <cmath>
#include <tuple>
#include <unordered_set>

#include "Utils/Constants.h"
#include "Utils/Math.h"
#include "Utils/Profiling.h"

void Physics::addRigidBody(std::weak_ptr<component::RigidBody> rigid_body)
{
    rigid_bodys_.push_back(rigid_body);
}

void Physics::addCollider(std::weak_ptr<component::Collider> collider, bool is_water)
{
    if (is_water)
        water_collider_ = collider;
    else
        colliders_.push_back(collider);
}

void Physics::update(float delta_time)
{
    ProfileScope;

    struct OverlapResolution
    {
        bool overlaps;
        glm::vec3 mtv;
    };

    const auto compute_aabb_resolution = [](const std::shared_ptr<component::Collider> &collider,
                                            const std::shared_ptr<component::Collider> &other_collider) {
        const auto &a = collider->aabb_;
        const auto &b = other_collider->aabb_;

        const float dx = b.center.x - a.center.x;
        const float dy = b.center.y - a.center.y;
        const float dz = b.center.z - a.center.z;

        const float px = (a.half_size.x + b.half_size.x) - std::abs(dx);
        const float py = (a.half_size.y + b.half_size.y) - std::abs(dy);
        const float pz = (a.half_size.z + b.half_size.z) - std::abs(dz);

        if (px <= 0.0f || py <= 0.0f || pz <= 0.0f)
        {
            return OverlapResolution{.overlaps = false, .mtv = glm::vec3(0.0f)};
        }

        const glm::vec3 axis_mask = collider->collision_resolution_mask_ * other_collider->collision_resolution_mask_;
        const bool allow_x = axis_mask.x > 0.0f;
        const bool allow_y = axis_mask.y > 0.0f;
        const bool allow_z = axis_mask.z > 0.0f;

        if (!allow_x && !allow_y && !allow_z)
        {
            return OverlapResolution{.overlaps = false, .mtv = glm::vec3(0.0f)};
        }

        const float best_x = allow_x ? px : std::numeric_limits<float>::max();
        const float best_y = allow_y ? py : std::numeric_limits<float>::max();
        const float best_z = allow_z ? pz : std::numeric_limits<float>::max();

        if (best_x <= best_y && best_x <= best_z)
        {
            const float sign = dx >= 0.0f ? 1.0f : -1.0f;
            return OverlapResolution{.overlaps = true, .mtv = glm::vec3(sign * px, 0.0f, 0.0f)};
        }
        if (best_y <= best_z)
        {
            const float sign = dy >= 0.0f ? 1.0f : -1.0f;
            return OverlapResolution{.overlaps = true, .mtv = glm::vec3(0.0f, sign * py, 0.0f)};
        }

        const float sign = dz >= 0.0f ? 1.0f : -1.0f;
        return OverlapResolution{.overlaps = true, .mtv = glm::vec3(0.0f, 0.0f, sign * pz)};
    };

    const auto get_resolving_rigid_body = [](const std::shared_ptr<component::Collider> &collider) {
        const auto rigid_body_opt = collider->getOwner()->getComponent<component::RigidBody>();
        if (!rigid_body_opt.has_value())
        {
            return std::shared_ptr<component::RigidBody>{};
        }

        if (!rigid_body_opt.value()->has_collisions_)
        {
            return std::shared_ptr<component::RigidBody>{};
        }

        return rigid_body_opt.value();
    };

    const auto inverse_mass_for_resolution = [](const std::shared_ptr<component::RigidBody> &rigid_body) {
        if (rigid_body == nullptr || rigid_body->is_static_)
        {
            return 0.0f;
        }

        return rigid_body->inverse_mass_;
    };

    const auto remove_velocity_along_inward_normal = [](const std::shared_ptr<component::RigidBody> &rigid_body,
                                                        const glm::vec3 &inward_normal) {
        if (rigid_body == nullptr || rigid_body->is_static_)
        {
            return;
        }

        const float inward_speed = glm::dot(rigid_body->velocity_, inward_normal);
        if (inward_speed > 0.0f)
        {
            rigid_body->velocity_ -= inward_speed * inward_normal;
        }
    };

    const auto apply_position_correction = [](const std::shared_ptr<component::RigidBody> &rigid_body,
                                              const std::shared_ptr<component::Collider> &collider,
                                              const glm::vec3 &correction) {
        if (rigid_body == nullptr || rigid_body->is_static_)
        {
            return;
        }

        rigid_body->position_ += correction;

        const auto transform = collider->transform_.lock();
        const auto world_position = glm::vec3(transform->resolve()[3]);
        transform->translate(rigid_body->position_ - world_position);
        transform->setRotation(rigid_body->orientation_);
        collider->update(0.0f);
    };

    std::erase_if(rigid_bodys_, [](const auto &wp) { return wp.expired(); });
    std::erase_if(colliders_, [](const auto &wp) { return wp.expired(); });

    std::unordered_set<std::shared_ptr<component::Collider>> had_water_collision;

    const auto water_collider = water_collider_.lock();

    for (const auto &rigid_body_ptr : rigid_bodys_)
    {
        auto rigid_body = rigid_body_ptr.lock();
        auto collider = rigid_body->collider_.lock();
        auto transform = collider->transform_.lock();

        if (!rigid_body->getOwner()->active)
        {
            continue;
        }

        if (rigid_body->is_static_)
        {
            continue;
        }

        if (collider->collideWith(*water_collider))
        {
            had_water_collision.insert(collider);

            // Archimedes' force simulation
            rigid_body->addForce([](const glm::vec3 &, const glm::vec3 &, const glm::vec3 &, const glm::quat &,
                                    float mass) { return std::make_tuple(mass * GRAVITY * UP, glm::vec3{}); });
        }

        // Gravity
        rigid_body->addForce([](const glm::vec3 &, const glm::vec3 &, const glm::vec3 &, const glm::quat &,
                                float mass) { return std::make_tuple(-mass * GRAVITY * UP, glm::vec3{}); });

        // Javelin stabilizer
        rigid_body->addForce([](const glm::vec3 &velocity, const glm::vec3 &, const glm::vec3 &angular_velocity,
                                const glm::quat &orientation, float) {
            const glm::vec3 forward = getForwardVector(orientation);

            const float tail_offset = 1.5f; // tune: distance from CM to tail
            const glm::vec3 application_point = -forward * tail_offset;

            const glm::vec3 v_tail = velocity + glm::cross(angular_velocity, application_point);

            const glm::vec3 v_perp = v_tail - glm::dot(v_tail, forward) * forward;

            const float drag_coeff = 5.0f; // tune: higher = faster alignment
            const glm::vec3 force = -drag_coeff * v_perp;

            return std::make_tuple(force, application_point);
        });

        rigid_body->updatePhysics(delta_time);

        const auto world_transform = transform->resolve();

        const auto world_position = glm::vec3(world_transform[3]);
        transform->translate(rigid_body->position_ - world_position);

        transform->setRotation(rigid_body->orientation_);
        collider->update(0.0f);
    }

    const auto water_id = water_collider->getOwner()->getId();
    std::vector<std::shared_ptr<GameObject>> to_detach;
    for (size_t i = 0; i < colliders_.size(); ++i)
    {
        const auto &collider = colliders_[i].lock();

        if (!collider->getOwner()->active)
        {
            continue;
        }

        if (collider->isDisabled())
            continue;

        if (had_water_collision.contains(collider))
        {
            if (collider->callCollisionCallbacks(water_id))
                to_detach.push_back(collider->getOwner());
        }

        for (size_t j = i + 1; j < colliders_.size(); ++j)
        {
            const auto &other_collider = colliders_[j].lock();

            if (other_collider->isDisabled())
                continue;

            if (collider->collideWith(*other_collider))
            {
                const auto rigid_body = get_resolving_rigid_body(collider);
                const auto other_rigid_body = get_resolving_rigid_body(other_collider);

                if (rigid_body != nullptr && other_rigid_body != nullptr)
                {
                    const auto overlap = compute_aabb_resolution(collider, other_collider);
                    if (overlap.overlaps)
                    {
                        const glm::vec3 normal = glm::normalize(overlap.mtv);

                        const float inverse_mass = inverse_mass_for_resolution(rigid_body);
                        const float other_inverse_mass = inverse_mass_for_resolution(other_rigid_body);
                        const float inverse_mass_sum = inverse_mass + other_inverse_mass;

                        if (inverse_mass_sum > 0.0f)
                        {
                            const glm::vec3 self_correction = -overlap.mtv * (inverse_mass / inverse_mass_sum);
                            const glm::vec3 other_correction = overlap.mtv * (other_inverse_mass / inverse_mass_sum);

                            apply_position_correction(rigid_body, collider, self_correction);
                            apply_position_correction(other_rigid_body, other_collider, other_correction);
                        }

                        remove_velocity_along_inward_normal(rigid_body, normal);
                        remove_velocity_along_inward_normal(other_rigid_body, -normal);
                    }
                }

                if (collider->callCollisionCallbacks(other_collider->getOwner()->getId()))
                    to_detach.push_back(collider->getOwner());
                if (other_collider->callCollisionCallbacks(collider->getOwner()->getId()))
                    to_detach.push_back(other_collider->getOwner());
            }
        }
    }

    for (const auto &game_object : to_detach)
    {
        game_object->detach();
    }
}

std::vector<glm::vec3> Physics::simulateCannonballTrajectory(const glm::vec3 &initial_position,
                                                             const glm::vec3 &initial_velocity)
{
    ProfileScope;

    constexpr const float DT = 0.05f;

    std::vector<glm::vec3> positions;
    positions.push_back(initial_position);

    glm::vec3 position = initial_position;
    glm::vec3 velocity = initial_velocity;

    while (glm::dot(position, UP) > 0.0f)
    {
        const auto acceleration = -GRAVITY * UP;
        velocity += acceleration * DT;
        position += velocity * DT;
        positions.push_back(position);
    }

    return positions;
}
