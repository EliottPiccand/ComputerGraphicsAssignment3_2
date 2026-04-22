#pragma once

#include "Components/Component.h"

namespace component
{

class FreeViewControls : public Component
{
  public:
    bool active = false;

    FreeViewControls();

    void update(float delta_time) override;
    bool render() const override;
};

} // namespace component
