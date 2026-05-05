#pragma once

#include <memory>
#include <variant>
#include <vector>

#include <Lib/glm.h>

#include "Components/Component.h"
#include "Components/Transform.h"
#include "GameObject.h"

class Physics;

namespace component
{

class RigidBody;

class Collider : public Component
{
  public:
    struct AABB
    {
        glm::vec3 half_size;
        glm::vec3 center;
    };

    struct ConvexPolyhedron
    {
        std::vector<glm::vec3> vertices;
        std::vector<glm::uvec3> faces;
    };

    struct Sphere
    {
        glm::vec3 center;
        float radius;
    };

    using Type = std::variant<AABB, ConvexPolyhedron, Sphere>;

    Collider(Type type, bool is_water = false);

    /// Component-wise scale for positional collision resolution axes.
    /// Example: (1, 1, 0) disables correction along UP.
    void setCollisionResolutionMask(const glm::vec3 &mask);
    [[nodiscard]] const glm::vec3 &getCollisionResolutionMask() const;

    void disable();
    [[nodiscard]] bool isDisabled();

    [[nodiscard]] bool collideWith(const Collider &other) const;
    [[nodiscard]] bool collideWithAABB(const Collider &other) const;

    /// return whether the object should be detached
    using CollisionCallback = std::function<bool(GameObjectId)>;
    void addCollisionCallback(CollisionCallback callback);
    /// return whether the object should be detached
    [[nodiscard]] bool callCollisionCallbacks(GameObjectId game_object_id) const;

    void initialize() override;
    void update() override;

  private:
    friend Physics;
    friend RigidBody;

    std::weak_ptr<Transform> transform_;

    Type type_;
    AABB aabb_;

    glm::vec3 collision_resolution_mask_;

    bool enabled_;

    bool is_water_;
    std::vector<CollisionCallback> collision_callbacks_;
};

} // namespace component
