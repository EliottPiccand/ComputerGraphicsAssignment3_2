#pragma once

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include <Lib/glm.h>

#include "Components/Collider.h"
#include "Components/Component.h"

class Physics;

namespace component
{

class RigidBody : public Component
{
  public:
    /// return a force and an application point
    using Force = std::function<std::pair<glm::vec3, glm::vec3>(const glm::vec3 &velocity, const glm::vec3 &position,
                                                                const glm::vec3 &angular_velocity,
                                                                const glm::quat &orientation, float mass)>;

    RigidBody();
    RigidBody(float mass);
    RigidBody(float mass, glm::mat3 inertia);

    void addForce(Force force);
    [[nodiscard]] const glm::vec3 &getVelocity() const;
    [[nodiscard]] const glm::vec3 &getPosition() const;
    void setVelocity(const glm::vec3 &velocity);
    void setOrientation(const glm::quat &orientation);

    void initialize() override;
    void reset();

    void updatePhysics(float delta_time);

  private:
    friend Physics;
    std::weak_ptr<Collider> collider_;

    bool is_static_;
    bool has_collisions_;

    float mass_;
    float inverse_mass_;
    glm::mat3 inverse_inertia_;

    std::vector<Force> forces_;

    glm::vec3 velocity_;
    glm::vec3 angular_velocity_;

    glm::vec3 position_;
    glm::quat orientation_;
};

} // namespace component
