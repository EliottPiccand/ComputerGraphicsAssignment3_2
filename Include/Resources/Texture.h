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
    static inline std::shared_ptr<Texture> MISSING_ALBEDO;
    static inline std::shared_ptr<Texture> MISSING_METALLIC_ROUGHNESS;
    static inline std::shared_ptr<Texture> MISSING_NORMAL_MAP;
    static inline constexpr const std::string_view DIRECTORY = "Textures";

    enum class Type
    {
        Albedo,
        MetallicRoughness,
        NormalMap,
        Emissive,
    };

    Texture(GLuint id, Type type);
    ~Texture();

    [[nodiscard]] static std::shared_ptr<Texture> load(const std::filesystem::path &path, Type type, bool is_hdr = false);

    void bind(GLenum slot, std::shared_ptr<resource::Shader> shader, const char *uniform_slot) const;
    [[nodiscard]] Type getType() const;

  private:
    const Type type_;
    const GLuint id_;
};

} // namespace resource
