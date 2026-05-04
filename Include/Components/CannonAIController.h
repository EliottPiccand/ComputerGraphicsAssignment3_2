#pragma once

#include <Lib/glm.h>

#include "Components/CannonController.h"
#include "Utils/Time.h"

namespace component
{

class CannonAIController : public CannonController
{
  public:
    CannonAIController(std::weak_ptr<Transform> cannon_barrel_transform);

    void initialize() override;

    void pickTargetTarget();
    void updateTarget(float delta_time) override;

  private:
    Instant last_fire_tick_;

    glm::vec3 target_target_;
};

} // namespace component
