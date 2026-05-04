#pragma once

#include <memory>

#include "Components/Camera3D.h"
#include "Components/CannonController.h"
#include "Components/Transform.h"

namespace component
{

class CannonPlayerController : public CannonController
{
  public:
    CannonPlayerController(std::weak_ptr<Transform> cannon_barrel_transform, std::weak_ptr<Camera3D> camera);

    void updateTarget(float delta_time) override;

  private:
    std::weak_ptr<Camera3D> camera_;
};

} // namespace component
