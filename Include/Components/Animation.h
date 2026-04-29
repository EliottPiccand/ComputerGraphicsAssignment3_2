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
    using Callback = std::function<void(float delta_time, std::shared_ptr<Transform> transform,
                                        std::shared_ptr<GameObject> game_object)>;

    Animation(Callback callback);

    void initialize() override;
    void update(float delta_time) override;

  private:
    std::weak_ptr<Transform> transform_;
    Callback callback_;
};

} // namespace component
