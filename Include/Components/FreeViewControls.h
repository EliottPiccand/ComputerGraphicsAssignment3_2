#pragma once

#include "Components/Component.h"

namespace component
{

class FreeViewControls : public Component
{
  public:
    bool active = false;

    FreeViewControls();

    void update() override;
};

} // namespace component
