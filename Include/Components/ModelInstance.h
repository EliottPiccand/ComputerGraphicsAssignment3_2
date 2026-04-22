#pragma once

#include <memory>

#include "Resources/Model.h"
#include "Components/Component.h"

namespace component
{

class ModelInstance : public Component
{
  public:
    ModelInstance(std::shared_ptr<resource::Model> model, resource::Model::TextureOverride texture_override = {});

    bool render() const override;

  private:
    std::shared_ptr<resource::Model> model_;
    resource::Model::TextureOverride texture_override_;
};

} // namespace component
