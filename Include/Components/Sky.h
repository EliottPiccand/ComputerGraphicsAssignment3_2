#pragma once

#include <memory>

#include <Lib/OpenGL.h>
#include <Lib/glm.h>
#include <vector>

#include "Components/Component.h"
#include "Resources/Shader.h"
#include "Resources/Texture.h"

namespace component
{

/// Must be attached to the scene root only
class Sky : public Component
{
  public:
    Sky(std::weak_ptr<resource::Texture> texture, std::vector<std::weak_ptr<resource::Shader>> shaders);

    void render(glm::mat4 &transform) const override;

  private:
    std::vector<std::weak_ptr<resource::Shader>> shaders_;
    std::weak_ptr<resource::Texture> texture_;
};

} // namespace component
