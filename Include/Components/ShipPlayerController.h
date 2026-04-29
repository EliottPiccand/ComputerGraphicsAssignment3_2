#pragma once

#include "Components/ShipController.h"

namespace component
{

class ShipPlayerController : public ShipController
{
  public:
    ShipPlayerController();

    void stop();

  protected:
    void updateStates() override;
};

} // namespace component
