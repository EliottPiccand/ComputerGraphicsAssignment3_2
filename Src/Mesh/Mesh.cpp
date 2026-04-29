#include "Mesh/Mesh.h"

#include <cmath>
#include <numbers>

#include "Utils/Constants.h"
#include "Utils/Log.h"
#include "Utils/Math.h"
#include "Utils/Profiling.h"

Mesh<VertexWater> generateQuadPlane(float side_length, size_t quads_per_side)
{
    ProfileScope;

    LOG_DEBUG("generating mesh: quad plane with side_length={:.3f} quads_per_side={}", side_length, quads_per_side);

    const size_t vertex_count = (quads_per_side + 1) * (quads_per_side + 1);
    const size_t degenerated_triangles_count = 2 * (quads_per_side - 1);

    std::vector<VertexWater> vertices;
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
            vertices.push_back({
                .position =
                    {
                        static_cast<float>(x) * quad_size - half_size,
                        static_cast<float>(y) * quad_size - half_size,
                        0.0f,
                    },
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

Mesh<VertexPBR> generateCylinder(float height, float radius, size_t resolution)
{
    ProfileScope;

    LOG_DEBUG("generating mesh: cylinder with height={:.3f} radius={:.3f} resolution={}", height, radius, resolution);

    const auto vertex_count = resolution * 4 + 2;
    const auto triangle_count = resolution * 4;

    std::vector<VertexPBR> vertices;
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

Mesh<VertexPBR> generateCone(float height, float radius, size_t resolution)
{
    ProfileScope;

    LOG_DEBUG("generating mesh: cone with height={:.3f} radius={:.3f} resolution={}", height, radius, resolution);

    const auto vertex_count = resolution * 2 + 2;
    const auto triangle_count = resolution * 2;

    std::vector<VertexPBR> vertices;
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

Mesh<VertexDebug> generateFrustrum(double near, double fov, double aspect_ratio)
{
    ProfileScope;

    LOG_DEBUG("generating mesh: frustrum with near={:.3f} fov={:.3f} aspect_ratio={:.3f}", near, fov, aspect_ratio);

    const auto near_center = MODEL_FORWARD * static_cast<float>(near) * 0.99f;
    const auto near_height = 2.0f * static_cast<float>(near) * glm::tan(glm::radians(static_cast<float>(fov) * 0.5f));
    const auto near_width = near_height * static_cast<float>(aspect_ratio);

    const auto half_width = MODEL_RIGHT * (near_width * 0.5f);
    const auto half_height = MODEL_UP * (near_height * 0.5f);

    const auto near_top_left = near_center - half_width + half_height;
    const auto near_top_right = near_center + half_width + half_height;
    const auto near_bottom_left = near_center - half_width - half_height;
    const auto near_bottom_right = near_center + half_width - half_height;

    const std::vector<VertexDebug> vertices = {
        {ZERO}, {near_top_left}, {near_top_right}, {near_bottom_left}, {near_bottom_right},
    };

    const std::vector<IndexType> indices = {
        0, 1, 0, 2, 0, 3, 0, 4, 1, 2, 2, 4, 4, 3, 3, 1,
    };

    return {vertices, indices};
}

Mesh<VertexUI> generateQuad()
{
    ProfileScope;

    const std::vector<VertexUI> vertices = {
        {
            .position = {-0.5f, 0.5f},
            .uv = {0.0f, 0.0f},
        },
        {
            .position = {-0.5f, -0.5f},
            .uv = {0.0f, 1.0f},
        },
        {
            .position = {0.5f, 0.5f},
            .uv = {1.0f, 0.0f},
        },
        {
            .position = {0.5f, -0.5f},
            .uv = {1.0f, 1.0f},
        },
    };

    const std::vector<IndexType> indices = {0, 1, 2, 2, 1, 3};
    return std::make_tuple(vertices, indices);
}

Mesh<VertexPBR> generateFlag(size_t strip_count, float width, float height, float uv_top, float uv_left,
                             float uv_bottom, float uv_right)
{
    ProfileScope;

    std::vector<VertexPBR> vertices;
    std::vector<IndexType> indices;

    constexpr const size_t VERTICES_PER_COLUMN = 4;

    vertices.reserve((strip_count + 1) * VERTICES_PER_COLUMN);
    indices.reserve(strip_count * 12);

    for (size_t i = 0; i <= strip_count; ++i)
    {
        const float u = static_cast<float>(i) / static_cast<float>(strip_count);
        const float x = u * width;
        const float uv_u = uv_left + u * (uv_right - uv_left);

        vertices.push_back(VertexPBR{
            .position = {x, -0.5f * height, 0.0f},
            .normal = {0.0f, 0.0f, 1.0f},
            .uv = {uv_u, uv_bottom},
        });
        vertices.push_back(VertexPBR{
            .position = {x, 0.5f * height, 0.0f},
            .normal = {0.0f, 0.0f, 1.0f},
            .uv = {uv_u, uv_top},
        });
        vertices.push_back(VertexPBR{
            .position = {x, -0.5f * height, 0.0f},
            .normal = {0.0f, 0.0f, -1.0f},
            .uv = {uv_u, uv_bottom},
        });
        vertices.push_back(VertexPBR{
            .position = {x, 0.5f * height, 0.0f},
            .normal = {0.0f, 0.0f, -1.0f},
            .uv = {uv_u, uv_top},
        });

        if (i < strip_count)
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

    return {vertices, indices};
}
