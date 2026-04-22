#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

#include <Lib/OpenGL.h>
#include <Lib/glm.h>

#include "Resources/Texture.h"
#include "Utils/Color.h"
#include "Utils/MeshPrimitives.h"

namespace resource
{

class Model
{
  private:
    struct Mesh_;

  public:
    static inline constexpr const std::string_view DIRECTORY = "Models";

    using TextureOverride = std::unordered_map<size_t, std::unordered_map<Texture::Type, std::shared_ptr<resource::Texture>>>;

    Model(GLuint vertex_array, GLuint vertex_buffer, std::vector<Mesh_> meshes);
    ~Model();

    [[nodiscard]] static std::shared_ptr<Model> loadFromFile(const std::filesystem::path &path);
    [[nodiscard]] static std::shared_ptr<Model> load(const Mesh &mesh, const Color &color);

    void draw(TextureOverride texture_override = {}) const;

  private:
    struct Material
    {
        std::shared_ptr<Texture> base_color_texture = nullptr;
        std::shared_ptr<Texture> metallic_roughness_texture = nullptr;
        std::shared_ptr<Texture> normal_texture = nullptr;
        std::shared_ptr<Texture> emissive_texture = nullptr;
        std::shared_ptr<Texture> ambient_occlusion_texture = nullptr;

        Color base_color = color::WHITE;
        float metallic_factor = 1.0f;
        float roughness = 1.0f;
        Color emissive_factor = color::TRANSPARENT;
    };

    struct Mesh_
    {
        GLuint index_buffer;
        GLsizei index_count;
        Material material;
    };

    const GLuint vertex_array_;
    const GLuint vertex_buffer_;

    const std::vector<Mesh_> meshes_;
};

} // namespace resource
