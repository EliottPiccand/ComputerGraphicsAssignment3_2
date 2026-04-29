#pragma once

#include <memory>

#include <Lib/glm.h>

#include "Components/Component.h"
#include "Resources/Texture.h"

namespace component
{

class Text : public Component
{
  public:
    Text(float width, float height, std::weak_ptr<resource::Texture> texture);

    void render(glm::mat4 &transform) const override;

  private:
    float width_;
    float height_;
    std::weak_ptr<resource::Texture> texture_;
};

} // namespace component
