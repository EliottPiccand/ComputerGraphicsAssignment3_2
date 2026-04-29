#pragma once

#include <memory>

#include <Lib/glm.h>

#include "Components/Component.h"
#include "Resources/Model.h"

namespace component
{

class ModelInstance : public Component
{
  public:
    ModelInstance(std::shared_ptr<resource::Model> model, resource::Model::TextureOverride texture_override = {});

    void render(glm::mat4 &transform) const override;

  private:
    std::shared_ptr<resource::Model> model_;
    resource::Model::TextureOverride texture_override_;
};

} // namespace component
