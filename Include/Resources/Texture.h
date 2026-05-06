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

    // clang-format off
    constexpr const static inline GLuint BASE_COLOR_SLOT         = 0;
    constexpr const static inline GLuint METALLIC_ROUGHNESS_SLOT = 1;
    constexpr const static inline GLuint NORMAL_MAP_SLOT         = 2;
    constexpr const static inline GLuint NORMAL_MAP_SLOT_2       = 3;
    constexpr const static inline GLuint EMISSIVE_SLOT           = 4;
    constexpr const static inline GLuint ENVIRONMENT_MAP_SLOT    = 5;
    constexpr const static inline GLuint COLOR_SLOT              = 6;
    constexpr const static inline GLuint DEPTH_SLOT              = 7;
    constexpr const static inline GLuint NOISE_MAP_SLOT          = 8;
    constexpr const static inline GLuint NOISE_MAP_SLOT_2        = 9;
    constexpr const static inline GLuint NORMALS_SLOT            = 10; /// framebuffer normals texture
    // clang-format on

    enum class Type
    {
        Albedo,
        MetallicRoughness,
        NormalMap,
        Emissive,
        Noise,
    };

    Texture(GLuint id, Type type);
    ~Texture();

    [[nodiscard]] static std::shared_ptr<Texture> load(const std::filesystem::path &path, Type type,
                                                       bool is_hdr = false);

    void bind(GLuint slot, std::shared_ptr<resource::Shader> shader, const char *uniform_slot) const;
    [[nodiscard]] Type getType() const;
    [[nodiscard]] GLuint getID() const;

  private:
    const Type type_;
    const GLuint id_;
};

} // namespace resource
