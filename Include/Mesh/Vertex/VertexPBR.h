#pragma once

#include <cstddef>
#include <functional>

#include <Lib/glm.h>

struct VertexPBR
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;

    static void setupVertexArray();

    friend bool operator==(const VertexPBR &, const VertexPBR &) = default;
};

namespace std
{

template <> struct hash<VertexPBR>
{
    size_t operator()(const VertexPBR &v) const noexcept
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
