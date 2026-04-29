#include "Components/Water.h"

#include <memory>

#include "Mesh/Mesh.h"
#include "Resources/ResourceLoader.h"
#include "Resources/Shader.h"
#include "Utils/Constants.h"
#include "Utils/Profiling.h"

using namespace component;

Water::Water()
{
    ProfileScope;

    constexpr const size_t QUADS_PER_SIDE = static_cast<size_t>(WORLD_WIDTH);

    const auto [vertices, indices] = generateQuadPlane(WORLD_WIDTH, QUADS_PER_SIDE);

    glGenVertexArrays(1, &vertex_array_);
    glBindVertexArray(vertex_array_);

    glGenBuffers(1, &vertex_buffer_);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(VertexWater)), vertices.data(),
                 GL_STATIC_DRAW);

    VertexWater::setupVertexArray();

    glGenBuffers(1, &index_buffer_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(IndexType)), indices.data(),
                 GL_STATIC_DRAW);
    index_count_ = static_cast<GLsizei>(indices.size());
}

void Water::render(glm::mat4 &transform) const
{
    ProfileScope;
    ProfileScopeGPU("Water::render");

    static std::weak_ptr weak_shader = ResourceLoader::getAsset<resource::Shader>("Water");

    auto shader = weak_shader.lock();
    shader->bind();
    shader->setUniform("u_Model", transform);

    glBindVertexArray(vertex_array_);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);

    glDrawElements(GL_TRIANGLE_STRIP, index_count_, GL_INDEX_TYPE, nullptr);
}
