#pragma once

#include <filesystem>
#include <memory>

#include <Lib/OpenGL.h>

namespace resource
{

class Texture
{
  public:
    static inline constexpr const std::string_view DIRECTORY = "Textures";

    enum class Type {
        Albedo,
        MetallicSmoothness,
        Normal,
        Emissive,
    };

    Texture(GLuint id);
    ~Texture();

    [[nodiscard]] static std::shared_ptr<Texture> loadFromFile(const std::filesystem::path &path);

    void bind(GLenum slot) const;
    void unbind(GLenum slot) const;

  private:
    const GLuint id_;
};

} // namespace resource
