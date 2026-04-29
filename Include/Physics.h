#pragma once

#include <memory>
#include <vector>

#include <Lib/glm.h>

#include "Components/Collider.h"
#include "Components/RigidBody.h"

class Physics
{
  public:
    static void addRigidBody(std::weak_ptr<component::RigidBody> rigid_body);
    static void addCollider(std::weak_ptr<component::Collider> collider, bool is_water = false);
    static void update(float delta_time);

    [[nodiscard]] static std::vector<glm::vec3> simulateCannonballTrajectory(const glm::vec3 &initial_position,
                                                                             const glm::vec3 &initial_velocity);

  private:
    static inline std::weak_ptr<component::Collider> water_collider_;
    static inline std::vector<std::weak_ptr<component::Collider>> colliders_;
    static inline std::vector<std::weak_ptr<component::RigidBody>> rigid_bodys_;
};
