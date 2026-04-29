#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <Lib/OpenGL.h>
#include <Lib/glm.h>

#include "Mesh/Vertex/VertexDebug.h"
#include "Mesh/Vertex/VertexPBR.h"
#include "Mesh/Vertex/VertexUI.h"
#include "Mesh/Vertex/VertexWater.h"

using IndexType = uint16_t;
constexpr const GLenum GL_INDEX_TYPE = GL_UNSIGNED_SHORT;

template <typename V> using Mesh = std::pair<std::vector<V>, std::vector<IndexType>>;

/// Plane facing +Z, indices made to be drawn as a triangle strip
[[nodiscard]] Mesh<VertexWater> generateQuadPlane(float side_length, size_t quads_per_side);

/// height along +Z, resolution is the amount of segments that each disk is approximated to
[[nodiscard]] Mesh<VertexPBR> generateCylinder(float height, float radius, size_t resolution);

/// height along +Z, resolution is the amount of segments that each disk is approximated to
[[nodiscard]] Mesh<VertexPBR> generateCone(float height, float radius, size_t resolution);

/// Respect MODEL_FORWARD, MODEL_RIGHT and MODEL_UP
[[nodiscard]] Mesh<VertexDebug> generateFrustrum(double near, double fov, double aspect_ratio = 16.0 / 9.0);

[[nodiscard]] Mesh<VertexUI> generateQuad();

[[nodiscard]] Mesh<VertexPBR> generateFlag(size_t strip_count, float width, float height, float uv_top = 0.0f,
                                           float uv_left = 0.0f, float uv_bottom = 0.0f, float uv_right = 0.0f);
