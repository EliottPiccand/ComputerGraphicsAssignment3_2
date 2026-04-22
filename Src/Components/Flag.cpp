#include "Components/Flag.h"

#include <cmath>
#include <numbers>
#include <vector>

#include "Utils/Color.h"

using namespace component;

constexpr const size_t STRIPS = 16;
constexpr const float WIDTH = 3.0f;
constexpr const float HEIGHT = 1.75f;

constexpr const float UV_LEFT = 0.17480f;
constexpr const float UV_TOP = 0.61816f;
constexpr const float UV_BOTTOM = 0.77689f;
constexpr const float UV_RIGHT = 0.34326f;

namespace
{

constexpr const float WAVE_BASE_AMPLITUDE = 0.25f;
constexpr const float WAVE_SECONDARY_AMPLITUDE = 0.08f;
constexpr const float WAVE_LENGTH = WIDTH * 0.8f;
constexpr const float WAVE_SPEED = 2.75f;

float animation_time_ = 0.0f;
std::vector<Vertex> base_vertices_;

float evaluateWave(float x, float time)
{
    const float u = glm::clamp(x / WIDTH, 0.0f, 1.0f);
    const float envelope = u * u;
    const float k = 2.0f * std::numbers::pi_v<float> / WAVE_LENGTH;

    const float primary = std::sin(k * x - WAVE_SPEED * time);
    const float secondary = std::sin(2.0f * k * x - 1.5f * WAVE_SPEED * time);

    return envelope * (WAVE_BASE_AMPLITUDE * primary + WAVE_SECONDARY_AMPLITUDE * secondary);
}

float evaluateWaveDerivative(float x, float time)
{
    const float u = glm::clamp(x / WIDTH, 0.0f, 1.0f);
    const float envelope = u * u;
    const float envelope_derivative = 2.0f * u / WIDTH;
    const float k = 2.0f * std::numbers::pi_v<float> / WAVE_LENGTH;

    const float phase_1 = k * x - WAVE_SPEED * time;
    const float phase_2 = 2.0f * k * x - 1.5f * WAVE_SPEED * time;

    const float oscillation = WAVE_BASE_AMPLITUDE * std::sin(phase_1) + WAVE_SECONDARY_AMPLITUDE * std::sin(phase_2);
    const float oscillation_derivative =
        WAVE_BASE_AMPLITUDE * k * std::cos(phase_1) + WAVE_SECONDARY_AMPLITUDE * 2.0f * k * std::cos(phase_2);

    return envelope_derivative * oscillation + envelope * oscillation_derivative;
}

} // namespace

Flag::Flag(std::shared_ptr<resource::Texture> texture) : texture_(texture)
{
}

void Flag::createMesh()
{
    std::vector<Vertex> vertices;
    std::vector<IndexType> indices;

    constexpr const size_t VERTICES_PER_COLUMN = 4;

    vertices.reserve((STRIPS + 1) * VERTICES_PER_COLUMN);
    indices.reserve(STRIPS * 12);

    for (size_t i = 0; i <= STRIPS; ++i)
    {
        const float u = static_cast<float>(i) / static_cast<float>(STRIPS);
        const float x = u * WIDTH;
        const float uv_u = UV_LEFT + u * (UV_RIGHT - UV_LEFT);

        vertices.push_back(Vertex{
            .position = {x, -0.5f * HEIGHT, 0.0f},
            .normal = {0.0f, 0.0f, 1.0f},
            .uv = {uv_u, UV_BOTTOM},
        });
        vertices.push_back(Vertex{
            .position = {x, 0.5f * HEIGHT, 0.0f},
            .normal = {0.0f, 0.0f, 1.0f},
            .uv = {uv_u, UV_TOP},
        });
        vertices.push_back(Vertex{
            .position = {x, -0.5f * HEIGHT, 0.0f},
            .normal = {0.0f, 0.0f, -1.0f},
            .uv = {uv_u, UV_BOTTOM},
        });
        vertices.push_back(Vertex{
            .position = {x, 0.5f * HEIGHT, 0.0f},
            .normal = {0.0f, 0.0f, -1.0f},
            .uv = {uv_u, UV_TOP},
        });

        if (i < STRIPS)
        {
            const IndexType front_bottom_left = static_cast<IndexType>(i * VERTICES_PER_COLUMN);
            const IndexType front_top_left = static_cast<IndexType>(i * VERTICES_PER_COLUMN + 1);
            const IndexType front_bottom_right = static_cast<IndexType>((i + 1) * VERTICES_PER_COLUMN);
            const IndexType front_top_right = static_cast<IndexType>((i + 1) * VERTICES_PER_COLUMN + 1);

            const IndexType back_bottom_left = static_cast<IndexType>(i * VERTICES_PER_COLUMN + 2);
            const IndexType back_top_left = static_cast<IndexType>(i * VERTICES_PER_COLUMN + 3);
            const IndexType back_bottom_right = static_cast<IndexType>((i + 1) * VERTICES_PER_COLUMN + 2);
            const IndexType back_top_right = static_cast<IndexType>((i + 1) * VERTICES_PER_COLUMN + 3);

            indices.push_back(front_bottom_left);
            indices.push_back(front_top_left);
            indices.push_back(front_top_right);

            indices.push_back(front_bottom_left);
            indices.push_back(front_top_right);
            indices.push_back(front_bottom_right);

            indices.push_back(back_bottom_left);
            indices.push_back(back_top_right);
            indices.push_back(back_top_left);

            indices.push_back(back_bottom_left);
            indices.push_back(back_bottom_right);
            indices.push_back(back_top_right);
        }
    }

    mesh_ = {std::move(vertices), std::move(indices)};
    base_vertices_ = mesh_.first;
    index_count_ = static_cast<GLsizei>(mesh_.second.size());
    animation_time_ = 0.0f;

    glGenVertexArrays(1, &vertex_array_);
    glBindVertexArray(vertex_array_);

    glGenBuffers(1, &vertex_buffer_);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh_.first.size() * sizeof(Vertex)), mesh_.first.data(),
                 GL_DYNAMIC_DRAW);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(Vertex), reinterpret_cast<const void *>(offsetof(Vertex, position)));

    glEnableClientState(GL_NORMAL_ARRAY);
    glNormalPointer(GL_FLOAT, sizeof(Vertex), reinterpret_cast<const void *>(offsetof(Vertex, normal)));

    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex), reinterpret_cast<const void *>(offsetof(Vertex, uv)));

    glGenBuffers(1, &index_buffer_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh_.second.size() * sizeof(IndexType)),
                 mesh_.second.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Flag::updateFlapping(float delta_time)
{
    if (mesh_.first.empty() || base_vertices_.size() != mesh_.first.size())
    {
        return;
    }

    animation_time_ += delta_time;

    constexpr const size_t VERTICES_PER_COLUMN = 4; // front(bottom,top) + back(bottom,top)

    for (size_t i = 0; i <= STRIPS; ++i)
    {
        const size_t front_bottom_idx = i * VERTICES_PER_COLUMN;
        const size_t front_top_idx = i * VERTICES_PER_COLUMN + 1;
        const size_t back_bottom_idx = i * VERTICES_PER_COLUMN + 2;
        const size_t back_top_idx = i * VERTICES_PER_COLUMN + 3;

        const float x = base_vertices_[front_bottom_idx].position.x;

        const float wave = evaluateWave(x, animation_time_);
        const float wave_derivative = evaluateWaveDerivative(x, animation_time_);
        const glm::vec3 front_normal = glm::normalize(glm::vec3(-wave_derivative, 0.0f, 1.0f));
        const glm::vec3 back_normal = -front_normal;

        mesh_.first[front_bottom_idx] = base_vertices_[front_bottom_idx];
        mesh_.first[front_top_idx] = base_vertices_[front_top_idx];
        mesh_.first[back_bottom_idx] = base_vertices_[back_bottom_idx];
        mesh_.first[back_top_idx] = base_vertices_[back_top_idx];

        mesh_.first[front_bottom_idx].position.z = wave;
        mesh_.first[front_top_idx].position.z = wave;
        mesh_.first[back_bottom_idx].position.z = wave;
        mesh_.first[back_top_idx].position.z = wave;

        mesh_.first[front_bottom_idx].normal = front_normal;
        mesh_.first[front_top_idx].normal = front_normal;
        mesh_.first[back_bottom_idx].normal = back_normal;
        mesh_.first[back_top_idx].normal = back_normal;
    }

    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh_.first.size() * sizeof(Vertex)), mesh_.first.data(),
                 GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

bool Flag::render() const
{
    const auto ambient = glm::vec4(glm::vec3(color::WHITE) * 0.5f, color::WHITE.w);
    GLfloat ambient_color[] = {_v4(ambient)};
    GLfloat diffuse_color[] = {_v4(color::WHITE)};
    GLfloat specular_color[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient_color);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse_color);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular_color);

    glEnable(GL_TEXTURE_2D);
    texture_->bind(GL_TEXTURE0);

    glBindVertexArray(vertex_array_);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);

    glDrawElements(GL_TRIANGLES, index_count_, GL_UNSIGNED_SHORT, nullptr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    texture_->unbind(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);

    return false;
}
