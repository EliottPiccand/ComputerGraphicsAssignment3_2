#pragma once

#include <memory>

#include <Lib/glm.h>

#include "Components/Component.h"
#include "Resources/Model.h"
#include "Resources/Texture.h"

namespace component
{

class Flag : public Component
{
  public:
    static constexpr const float WIDTH = 3.0f;

    Flag(std::weak_ptr<resource::Texture> texture);

    void update(float delta_time) override;
    void render(glm::mat4 &transform) const override;

  private:
    float animation_time_;

    resource::Model::MaterialsOverride materials_override_;
};

} // namespace component
