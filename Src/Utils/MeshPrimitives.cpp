#include "Utils/MeshPrimitives.h"

#include <cmath>
#include <numbers>

#include "Utils/Constants.h"

Mesh generateQuadPlane(float side_length, size_t quads_per_side)
{
    const size_t vertex_count = (quads_per_side + 1) * (quads_per_side + 1);
    const size_t degenerated_triangles_count = 2 * (quads_per_side - 1);

    std::vector<Vertex> vertices;
    vertices.reserve(vertex_count);
    std::vector<IndexType> indices;
    indices.reserve(vertex_count + degenerated_triangles_count);

    float half_size = 0.5f * side_length;
    float quad_size = side_length / static_cast<float>(quads_per_side);

    // Generate vertices
    for (size_t y = 0; y <= quads_per_side; ++y)
    {
        for (size_t x = 0; x <= quads_per_side; ++x)
        {
            const float u = static_cast<float>(x) / static_cast<float>(quads_per_side);
            const float v = static_cast<float>(y) / static_cast<float>(quads_per_side);
            vertices.push_back({
                .position =
                    {
                        static_cast<float>(x) * quad_size - half_size,
                        static_cast<float>(y) * quad_size - half_size,
                        0.0f,
                    },
                .normal = Z,
                .uv = {u, v},
            });
        }
    }

    // Generate indices
    for (size_t y = 0; y < quads_per_side; ++y)
    {
        if (y > 0)
        {
            // Degenerate: repeat first vertex of new row
            indices.push_back(static_cast<IndexType>(y * (quads_per_side + 1)));
        }
        for (size_t x = 0; x <= quads_per_side; ++x)
        {
            indices.push_back(static_cast<IndexType>((y + 1) * (quads_per_side + 1) + x));
            indices.push_back(static_cast<IndexType>(y * (quads_per_side + 1) + x));
        }
        if (y < quads_per_side - 1)
        {
            // Degenerate: repeat last vertex of current row
            indices.push_back(static_cast<IndexType>(((y + 1) * (quads_per_side + 1) + quads_per_side)));
        }
    }

    return {vertices, indices};
}

Mesh generateCylinder(float height, float radius, size_t resolution)
{
    const auto vertex_count = resolution * 4 + 2;
    const auto triangle_count = resolution * 4;

    std::vector<Vertex> vertices;
    vertices.reserve(vertex_count);
    std::vector<IndexType> indices;
    indices.reserve(triangle_count * 3);

    const float half_height = height / 2.0f;

    // Generate vertices
    for (size_t i = 0; i < resolution; ++i)
    {
        const float arg = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(resolution);
        const float x = radius * std::cos(arg);
        const float y = radius * std::sin(arg);
        const float u = static_cast<float>(i) / static_cast<float>(resolution);
        const glm::vec3 radial_normal =
            radius > 0.0f ? glm::normalize(glm::vec3{x, y, 0.0f}) : glm::vec3{1.0f, 0.0f, 0.0f};
        const glm::vec2 cap_uv =
            radius > 0.0f ? glm::vec2{0.5f + x / (2.0f * radius), 0.5f + y / (2.0f * radius)} : glm::vec2{0.5f, 0.5f};

        vertices.push_back({
            .position = {x, y, -half_height},
            .normal = radial_normal,
            .uv = {u, 0.0f},
        });
        vertices.push_back({
            .position = {x, y, half_height},
            .normal = radial_normal,
            .uv = {u, 1.0f},
        });
        vertices.push_back({
            .position = {x, y, -half_height},
            .normal = {0.0f, 0.0f, -1.0f},
            .uv = cap_uv,
        });
        vertices.push_back({
            .position = {x, y, half_height},
            .normal = {0.0f, 0.0f, 1.0f},
            .uv = cap_uv,
        });
    }

    const auto bottom_center = static_cast<IndexType>(vertex_count - 2);
    const auto top_center = static_cast<IndexType>(vertex_count - 1);

    // disks' center vertices
    vertices.push_back({
        .position = {0.0f, 0.0f, -half_height},
        .normal = {0.0f, 0.0f, -1.0f},
        .uv = {0.5f, 0.5f},
    });
    vertices.push_back({
        .position = {0.0f, 0.0f, half_height},
        .normal = {0.0f, 0.0f, 1.0f},
        .uv = {0.5f, 0.5f},
    });

    // Generate indices
    for (size_t i = 0; i < resolution; ++i)
    {
        const auto next = (i + 1) % resolution;

        const IndexType side_bottom_current = static_cast<IndexType>(4 * i);
        const IndexType side_top_current = static_cast<IndexType>(4 * i + 1);
        const IndexType bottom_current = static_cast<IndexType>(4 * i + 2);
        const IndexType top_current = static_cast<IndexType>(4 * i + 3);
        const IndexType side_bottom_next = static_cast<IndexType>(4 * next);
        const IndexType side_top_next = static_cast<IndexType>(4 * next + 1);
        const IndexType bottom_next = static_cast<IndexType>(4 * next + 2);
        const IndexType top_next = static_cast<IndexType>(4 * next + 3);

        // Bottom cap faces downward (-Z).
        indices.push_back(bottom_center);
        indices.push_back(bottom_next);
        indices.push_back(bottom_current);

        // Top cap faces upward (+Z).
        indices.push_back(top_center);
        indices.push_back(top_current);
        indices.push_back(top_next);

        // Side faces.
        indices.push_back(side_bottom_current);
        indices.push_back(side_bottom_next);
        indices.push_back(side_top_next);

        indices.push_back(side_bottom_current);
        indices.push_back(side_top_next);
        indices.push_back(side_top_current);
    }

    return {vertices, indices};
}

Mesh generateCone(float height, float radius, size_t resolution)
{
    const auto vertex_count = resolution * 2 + 2;
    const auto triangle_count = resolution * 2;

    std::vector<Vertex> vertices;
    vertices.reserve(vertex_count);
    std::vector<IndexType> indices;
    indices.reserve(triangle_count * 3);

    const glm::vec3 apex_position = {0.0f, 0.0f, 0.0f};

    for (size_t i = 0; i < resolution; ++i)
    {
        const float arg = 2.0f * std::numbers::pi_v<float> * static_cast<float>(i) / static_cast<float>(resolution);
        const float x = radius * std::cos(arg);
        const float y = radius * std::sin(arg);
        const float u = static_cast<float>(i) / static_cast<float>(resolution);

        const glm::vec3 side_edge = glm::vec3{x, y, height} - apex_position;
        glm::vec3 side_normal = glm::cross(glm::vec3{-y, x, 0.0f}, side_edge);
        const float side_normal_length = glm::length(side_normal);
        if (side_normal_length > 0.0f)
        {
            side_normal /= side_normal_length;
        }
        else
        {
            side_normal = {1.0f, 0.0f, 0.0f};
        }

        const glm::vec2 cap_uv =
            radius > 0.0f ? glm::vec2{0.5f + x / (2.0f * radius), 0.5f + y / (2.0f * radius)} : glm::vec2{0.5f, 0.5f};

        vertices.push_back({
            .position = {x, y, height},
            .normal = side_normal,
            .uv = {u, 0.0f},
        });
        vertices.push_back({
            .position = {x, y, height},
            .normal = {0.0f, 0.0f, 1.0f},
            .uv = cap_uv,
        });
    }

    const auto apex = static_cast<IndexType>(vertex_count - 2);
    const auto bottom_center = static_cast<IndexType>(vertex_count - 1);

    vertices.push_back({
        .position = apex_position,
        .normal = {0.0f, 0.0f, -1.0f},
        .uv = {0.5f, 1.0f},
    });
    vertices.push_back({
        .position = {0.0f, 0.0f, height},
        .normal = {0.0f, 0.0f, 1.0f},
        .uv = {0.5f, 0.5f},
    });

    for (size_t i = 0; i < resolution; ++i)
    {
        const auto next = (i + 1) % resolution;

        const IndexType side_current = static_cast<IndexType>(2 * i);
        const IndexType bottom_current = static_cast<IndexType>(2 * i + 1);
        const IndexType side_next = static_cast<IndexType>(2 * next);
        const IndexType bottom_next = static_cast<IndexType>(2 * next + 1);

        // Side faces point outward.
        indices.push_back(side_current);
        indices.push_back(apex);
        indices.push_back(side_next);

        // Base cap faces upward (+Z).
        indices.push_back(bottom_center);
        indices.push_back(bottom_current);
        indices.push_back(bottom_next);
    }

    return {vertices, indices};
}
