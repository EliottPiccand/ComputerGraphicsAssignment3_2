#pragma once

#include <filesystem>
#include <memory>
#include <string_view>

#include <Lib/OpenGL.h>
#include <Lib/glm.h>

#include "Resources/Shader.h"

namespace resource
{

class ComputeShader : public Shader
{
  public:
    static inline constexpr const std::string_view DIRECTORY = "Shaders/Compute";

    [[nodiscard]] static std::shared_ptr<ComputeShader> load(const std::filesystem::path &path,
                                                             const Defines defines = {});
};

} // namespace resource
