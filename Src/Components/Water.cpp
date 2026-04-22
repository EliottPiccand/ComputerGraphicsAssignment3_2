#include "Components/Water.h"

#include "Utils/Constants.h"
#include "Utils/MeshPrimitives.h"
#include "Utils/Profiling.h"

using namespace component;

Water::Water()
{
    constexpr const size_t QUADS_PER_SIDE = static_cast<size_t>(WORLD_WIDTH);

    const auto [vertices, indices] = generateQuadPlane(WORLD_WIDTH, QUADS_PER_SIDE);

    glGenVertexArrays(1, &vertex_array_);
    glBindVertexArray(vertex_array_);

    glGenBuffers(1, &vertex_buffer_);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(),
                 GL_STATIC_DRAW);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(Vertex), reinterpret_cast<const void *>(offsetof(Vertex, position)));

    glGenBuffers(1, &index_buffer_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(IndexType)), indices.data(),
                 GL_STATIC_DRAW);
    index_count_ = static_cast<GLsizei>(indices.size());

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

bool Water::render() const
{
    ProfileScope;
    ProfileScopeGPU("Water::render");

    constexpr const GLfloat material_ambient[] = {0.0f, 0.1f, 0.3f, 1.0f};
    constexpr const GLfloat material_diffuse[] = {0.0f, 0.4f, 0.8f, 0.6f};

    glMaterialfv(GL_FRONT, GL_AMBIENT, material_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, material_diffuse);

    glBindVertexArray(vertex_array_);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);

    glDrawElements(GL_TRIANGLE_STRIP, index_count_, GL_UNSIGNED_SHORT, nullptr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return false;
}
