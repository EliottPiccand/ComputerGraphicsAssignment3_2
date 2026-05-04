#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <Lib/OpenGL.h>
#include <Lib/glm.h>

#include "Mesh/Mesh.h"
#include "Mesh/Vertex/Vertex.h"
#include "Resources/Shader.h"
#include "Resources/Texture.h"
#include "Utils/Color.h"

namespace resource
{

class Model
{
  private:
    struct Shape;

  public:
    static inline constexpr const std::string_view DIRECTORY = "Models";

    struct MaterialOverride
    {
        // clang-format off
        std::optional<Color> albedo_color                                   = std::nullopt;
        std::optional<std::weak_ptr<Texture>> albedo_texture                = std::nullopt;

        /// 0 → 1 = Dielectric → Metal
        std::optional<float> metallic_factor                                = std::nullopt;
        /// 0 → 1 = Mirror → Matte
        std::optional<float> roughness_factor                               = std::nullopt;
        /// R: unused | G: roughness | B: metallic
        std::optional<std::weak_ptr<Texture>> metallic_roughness_texture    = std::nullopt;

        std::optional<std::weak_ptr<Texture>> normal_map                    = std::nullopt;

        std::optional<Color> emissive_color                                 = std::nullopt;
        std::optional<std::weak_ptr<Texture>> emissive_texture              = std::nullopt;

        std::optional<float> specular_factor                                = std::nullopt;
        std::optional<Color> specular_color                                 = std::nullopt;

        // clang-format on
    };

    using MaterialsOverride = std::unordered_map<size_t, MaterialOverride>;

    Model(GLuint vertex_array, GLuint vertex_buffer, std::vector<Shape> meshes);
    ~Model();

    [[nodiscard]] static std::shared_ptr<Model> load(const std::filesystem::path &path);

    template <Vertex T>
    [[nodiscard]]
    static std::shared_ptr<Model> load(const Mesh<T> &mesh, const Color &color = color::WHITE);

    void bind() const;
    [[nodiscard]] GLsizei getIndexCount() const;

    void draw(std::shared_ptr<resource::Shader> shader, MaterialsOverride materials_override = {}) const;
    void drawInstanced(std::shared_ptr<resource::Shader> shader, size_t intance_count,
                       MaterialsOverride materials_override = {}) const;

  private:
    struct Material
    {
        Color albedo_color = color::WHITE;
        std::weak_ptr<Texture> albedo_texture = std::shared_ptr<Texture>(nullptr);

        /// 0 → 1 = Dielectric → Metal
        float metallic_factor = 1.0f;
        /// 0 → 1 = Mirror → Matte
        float roughness_factor = 1.0f;
        /// R: unused | G: roughness | B: metallic
        std::weak_ptr<Texture> metallic_roughness_texture = std::shared_ptr<Texture>(nullptr);

        std::weak_ptr<Texture> normal_map = std::shared_ptr<Texture>(nullptr);

        Color emissive_color = color::TRANSPARENT;
        std::weak_ptr<Texture> emissive_texture = std::shared_ptr<Texture>(nullptr);

        float specular_factor = 1.0f;
        Color specular_color = color::WHITE;
    };

    struct Shape
    {
        GLuint index_buffer;
        GLsizei index_count;
        Material material;
    };

    const GLuint vertex_array_;
    const GLuint vertex_buffer_;

    const std::vector<Shape> shapes_;

    void drawInner(std::shared_ptr<resource::Shader> shader, MaterialsOverride materials_override,
                   std::optional<size_t> intance_count) const;
};

template <Vertex T> std::shared_ptr<Model> Model::load(const Mesh<T> &mesh, const Color &color)
{
    const auto &[vertices, indices] = mesh;

    GLuint vertex_array;
    glGenVertexArrays(1, &vertex_array);
    glBindVertexArray(vertex_array);

    GLuint vertex_buffer;
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(T)), vertices.data(),
                 GL_STATIC_DRAW);

    T::setupVertexArray();

    GLuint index_buffer;
    glGenBuffers(1, &index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(IndexType)), indices.data(),
                 GL_STATIC_DRAW);

    std::vector<Shape> meshes;
    meshes.push_back({
        .index_buffer = index_buffer,
        .index_count = static_cast<GLsizei>(indices.size()),
        .material =
            {
                .albedo_color = color,
            },
    });

    return std::make_shared<Model>(vertex_array, vertex_buffer, meshes);
}

} // namespace resource
