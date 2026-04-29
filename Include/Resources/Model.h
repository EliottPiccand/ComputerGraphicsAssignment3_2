#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
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

    using TextureOverride =
        std::unordered_map<size_t, std::unordered_map<Texture::Type, std::shared_ptr<resource::Texture>>>;

    Model(GLuint vertex_array, GLuint vertex_buffer, std::vector<Shape> meshes);
    ~Model();

    [[nodiscard]] static std::shared_ptr<Model> loadFromFile(const std::filesystem::path &path);

    template <Vertex T>
    [[nodiscard]]
    static std::shared_ptr<Model> load(const Mesh<T> &mesh, const Color &color = color::WHITE);

    void bind() const;
    [[nodiscard]] GLsizei getIndexCount() const;

    void draw(std::shared_ptr<resource::Shader> shader, TextureOverride texture_override = {}) const;
    void drawInstanced(std::shared_ptr<resource::Shader> shader, size_t intance_count,
                       TextureOverride texture_override = {}) const;

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
        float roughness_factor = 1.0f;
        Color emissive_color = color::TRANSPARENT;
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
                .base_color = color,
            },
    });

    return std::make_shared<Model>(vertex_array, vertex_buffer, meshes);
}

} // namespace resource
