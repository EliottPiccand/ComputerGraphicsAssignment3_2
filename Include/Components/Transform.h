#pragma once

#include <Lib/glm.h>

#include "Components/Component.h"

namespace component
{

class Transform : public Component
{
  public:
    Transform(const glm::vec3 &position, const glm::vec3 &rotation, const glm::vec3 &scale);
    Transform(const glm::vec3 &position, const glm::vec3 &rotation);
    Transform(const glm::vec3 &position);
    Transform();

    [[nodiscard]] glm::mat4 resolve() const;

    void translate(const glm::vec3 &by);
    /// axis should be normalized
    void rotate(const float angle, const glm::vec3 &axis);

    void setPosition(const glm::vec3 &position);
    void setRotation(const glm::quat &rotation);
    void setScale(const glm::vec3 &scale);

    [[nodiscard]] glm::vec3 getPosition() const;
    [[nodiscard]] glm::quat getRotation() const;
    [[nodiscard]] glm::vec3 getScale() const;

    /// axis should be normalized
    void pointToward(const glm::vec3 &direction);

    void render(glm::mat4 &transform) const override;
    void renderDefered(glm::mat4 &transform) const override;

  private:
    glm::vec3 position_;
    glm::quat rotation_;
    glm::vec3 scale_;
};

} // namespace component
