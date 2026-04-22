#pragma once

#include <memory>

#include <Lib/glm.h>

#include "Components/ShipController.h"
#include "Components/Transform.h"


namespace component
{

class ShipAIController : public ShipController
{
  public:
    ShipAIController(std::weak_ptr<Transform> target_transforù);

    bool render() const override;

  protected:
    void initialize() override;
    void updateStates() override;

  private:
    void pickTarget();

    std::weak_ptr<Transform> target_transform_;
    glm::vec2 target_;
};

} // namespace component
