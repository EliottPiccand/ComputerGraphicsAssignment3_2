#pragma once

#include <filesystem>
#include <memory>

#include <Lib/OpenGL.h>
#include <Lib/glm.h>

#include "Resources/Shader.h"

namespace resource
{

class ComputeShader : public Shader
{
  public:
    [[nodiscard]] static std::shared_ptr<ComputeShader> loadFromFile(const std::filesystem::path &path);
};

} // namespace resource
