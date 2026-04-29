#pragma once

#include <memory>

#include <Lib/glm.h>

#include "Components/Component.h"
#include "Resources/Texture.h"

namespace component
{

class Flag : public Component
{
  public:
    Flag(std::weak_ptr<resource::Texture> texture);

    void update(float delta_time) override;
    void render(glm::mat4 &transform) const override;

  private:
    float animation_time_;

    std::weak_ptr<resource::Texture> texture_;
};

} // namespace component
