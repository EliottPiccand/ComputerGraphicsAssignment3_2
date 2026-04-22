#pragma once

#include <memory>

#include "Component.h"
#include "Resources/Texture.h"

namespace component
{

class Text : public Component
{
  public:
    Text(float width, float height, std::shared_ptr<resource::Texture> texture);

    bool render() const override;

  private:
    float width_;
    float height_;
    std::shared_ptr<resource::Texture> texture_;
};

} // namespace component
