#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

#include <Lib/glm.h>

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;

    friend bool operator==(const Vertex &, const Vertex &) = default;
};

using IndexType = uint16_t;

using Mesh = std::pair<std::vector<Vertex>, std::vector<IndexType>>;

/// Plane facing +Z, indices made to be drawn as a triangle strip
Mesh generateQuadPlane(float side_length, size_t quads_per_side);

/// height along +Z, resolution is the amount of segments that each disk is approximated to
Mesh generateCylinder(float height, float radius, size_t resolution);

/// height along +Z, resolution is the amount of segments that each disk is approximated to
Mesh generateCone(float height, float radius, size_t resolution);

namespace std
{

template <> struct hash<Vertex>
{
    size_t operator()(const Vertex &v) const noexcept
    {
        const size_t h1 = std::hash<float>{}(v.position.x);
        const size_t h2 = std::hash<float>{}(v.position.y);
        const size_t h3 = std::hash<float>{}(v.position.z);

        const size_t h4 = std::hash<float>{}(v.normal.x);
        const size_t h5 = std::hash<float>{}(v.normal.y);
        const size_t h6 = std::hash<float>{}(v.normal.z);

        const size_t h7 = std::hash<float>{}(v.uv.x);
        const size_t h8 = std::hash<float>{}(v.uv.y);

        size_t seed = h1;
        seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h4 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h5 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h6 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h7 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= h8 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

} // namespace std
