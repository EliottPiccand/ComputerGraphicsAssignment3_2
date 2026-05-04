#pragma once

#include <functional>
#include <memory>

#include "Components/Component.h"
#include "Components/Transform.h"
#include "GameObject.h"

namespace component
{

class Animation : public Component
{
  public:
    using Callback = std::function<void(std::shared_ptr<Transform> transform, std::shared_ptr<GameObject> game_object)>;

    Animation(Callback callback);

    void initialize() override;
    void update() override;

  private:
    std::weak_ptr<Transform> transform_;
    Callback callback_;
};

} // namespace component
