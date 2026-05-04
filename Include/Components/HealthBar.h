#pragma once

#include <memory>

#include "Components/Component.h"
#include "Components/Health.h"
#include "Components/Transform.h"

namespace component
{

class HealthBar : public Component
{
  public:
    HealthBar(std::weak_ptr<Health> health, std::weak_ptr<Transform> follow_target);

    void initialize() override;
    void update() override;
    void render(glm::mat4 &transform) const override;

  private:
    std::weak_ptr<Health> health_;
    std::weak_ptr<Transform> follow_target_;
    std::weak_ptr<Transform> transform_;
};

} // namespace component
