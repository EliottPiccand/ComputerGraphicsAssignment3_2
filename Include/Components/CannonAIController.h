#pragma once

#include <memory>

#include <Lib/glm.h>

#include "Components/CannonController.h"
#include "Components/Transform.h"
#include "Utils/Time.h"

namespace component
{

class CannonAIController : public CannonController
{
  public:
    CannonAIController(std::weak_ptr<Transform> cannon_barrel_transform, std::weak_ptr<Transform> target_transform,
                       std::weak_ptr<Transform> target_target_transform);

    void initialize() override;

    void pickTargetTarget();
    void updateTarget() override;

  private:
    Instant last_fire_tick_;

    glm::vec3 target_target_;
    std::weak_ptr<Transform> target_target_transform_;
};

} // namespace component
