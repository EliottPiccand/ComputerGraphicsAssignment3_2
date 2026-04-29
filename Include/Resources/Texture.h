#pragma once

#include <filesystem>
#include <memory>
#include <string_view>

#include <Lib/OpenGL.h>

#include "Resources/Shader.h"

namespace resource
{

class Texture
{
  public:
    static inline std::shared_ptr<Texture> MISSING;
    static inline constexpr const std::string_view DIRECTORY = "Textures";

    enum class Type
    {
        Albedo,
        MetallicRoughness,
        Normal,
        Emissive,
    };

    Texture(GLuint id);
    ~Texture();

    [[nodiscard]] static std::shared_ptr<Texture> loadFromFile(const std::filesystem::path &path);

    void bind(GLenum slot, std::shared_ptr<resource::Shader> shader, const char *uniform_slot) const;

  private:
    const GLuint id_;
};

} // namespace resource
