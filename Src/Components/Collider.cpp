#include "Components/Collider.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>

#include <Lib/OpenGL.h>

#include "GameObject.h" // IWYU pragma: keep
#include "Physics.h"
#include "Singleton.h"
#include "Utils/Color.h"
#include "Utils/Math.h"

using namespace component;

namespace
{

Collider::AABB computeAabb(const std::vector<glm::vec3> &vertices, const glm::mat4 &transform)
{
    if (vertices.empty())
    {
        throw std::runtime_error("convex polyhedron collider needs at least one vertex");
    }

    glm::vec3 min;
    glm::vec3 max;

    for (size_t i = 0; i < vertices.size(); ++i)
    {
        const auto world_vertex = glm::vec3(transform * glm::vec4(vertices[i], 1.0f));
        if (i == 0)
        {
            min = world_vertex;
            max = world_vertex;
        }
        else
        {
            min.x = glm::min(min.x, world_vertex.x);
            min.y = glm::min(min.y, world_vertex.y);
            min.z = glm::min(min.z, world_vertex.z);
            max.x = glm::max(max.x, world_vertex.x);
            max.y = glm::max(max.y, world_vertex.y);
            max.z = glm::max(max.z, world_vertex.z);
        }
    }

    return {.half_size = (max - min) * 0.5f, .center = (max + min) * 0.5f};
}

Collider::ConvexPolyhedron transformPolyhedron(const Collider::ConvexPolyhedron &polyhedron, const glm::mat4 &transform)
{
    Collider::ConvexPolyhedron transformed = polyhedron;
    for (auto &vertex : transformed.vertices)
    {
        vertex = glm::vec3(transform * glm::vec4(vertex, 1.0f));
    }
    return transformed;
}

std::pair<glm::vec3, float> transformSphere(const Collider::Sphere &sphere, const glm::mat4 &transform)
{
    const auto world_center = glm::vec3(transform * glm::vec4(sphere.center, 1.0f));
    const float scale_x = glm::length(glm::vec3(transform[0]));
    const float scale_y = glm::length(glm::vec3(transform[1]));
    const float scale_z = glm::length(glm::vec3(transform[2]));
    const float world_radius = sphere.radius * glm::max(scale_x, glm::max(scale_y, scale_z));

    return {world_center, world_radius};
}

glm::vec3 closestPointOnTriangle(const glm::vec3 &point,
                                 const glm::vec3 &a,
                                 const glm::vec3 &b,
                                 const glm::vec3 &c)
{
    const glm::vec3 ab = b - a;
    const glm::vec3 ac = c - a;
    const glm::vec3 ap = point - a;

    const float d1 = glm::dot(ab, ap);
    const float d2 = glm::dot(ac, ap);
    if (d1 <= 0.0f && d2 <= 0.0f)
    {
        return a;
    }

    const glm::vec3 bp = point - b;
    const float d3 = glm::dot(ab, bp);
    const float d4 = glm::dot(ac, bp);
    if (d3 >= 0.0f && d4 <= d3)
    {
        return b;
    }

    const float vc = d1 * d4 - d3 * d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
    {
        const float v = d1 / (d1 - d3);
        return a + v * ab;
    }

    const glm::vec3 cp = point - c;
    const float d5 = glm::dot(ab, cp);
    const float d6 = glm::dot(ac, cp);
    if (d6 >= 0.0f && d5 <= d6)
    {
        return c;
    }

    const float vb = d5 * d2 - d1 * d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
    {
        const float w = d2 / (d2 - d6);
        return a + w * ac;
    }

    const float va = d3 * d6 - d5 * d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
    {
        const float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
        return b + w * (c - b);
    }

    const float denom = 1.0f / (va + vb + vc);
    const float v = vb * denom;
    const float w = vc * denom;
    return a + ab * v + ac * w;
}

bool collideSphereAabb(const glm::vec3 &sphere_center, float sphere_radius, const Collider::AABB &aabb)
{
    const glm::vec3 aabb_min = aabb.center - aabb.half_size;
    const glm::vec3 aabb_max = aabb.center + aabb.half_size;
    const glm::vec3 closest_point = glm::clamp(sphere_center, aabb_min, aabb_max);
    return glm::dot(closest_point - sphere_center, closest_point - sphere_center) <= sphere_radius * sphere_radius;
}

bool collideSpherePolyhedron(const glm::vec3 &sphere_center, float sphere_radius, const Collider::ConvexPolyhedron &polyhedron)
{
    if (polyhedron.vertices.empty() || polyhedron.faces.empty())
    {
        return false;
    }

    const float radius_sq = sphere_radius * sphere_radius;
    float closest_distance_sq = std::numeric_limits<float>::max();

    for (const auto &face : polyhedron.faces)
    {
        const auto &v0 = polyhedron.vertices[face.x];
        const auto &v1 = polyhedron.vertices[face.y];
        const auto &v2 = polyhedron.vertices[face.z];

        const auto closest_point = closestPointOnTriangle(sphere_center, v0, v1, v2);
        closest_distance_sq = glm::min(closest_distance_sq,
                           glm::dot(closest_point - sphere_center, closest_point - sphere_center));

        if (closest_distance_sq <= radius_sq)
        {
            return true;
        }
    }

    return closest_distance_sq <= radius_sq;
}

void appendFaceAxes(const Collider::ConvexPolyhedron &polyhedron, std::vector<glm::vec3> &axes)
{
    for (const auto &face : polyhedron.faces)
    {
        const auto &v0 = polyhedron.vertices[face.x];
        const auto &v1 = polyhedron.vertices[face.y];
        const auto &v2 = polyhedron.vertices[face.z];

        const auto normal = glm::cross(v1 - v0, v2 - v0);
        if (glm::dot(normal, normal) > EPSILON * EPSILON)
        {
            axes.push_back(glm::normalize(normal));
        }
    }
}

std::vector<glm::vec3> collectEdgeDirections(const Collider::ConvexPolyhedron &polyhedron)
{
    std::vector<glm::vec3> edges;
    edges.reserve(polyhedron.faces.size() * 3);

    for (const auto &face : polyhedron.faces)
    {
        const auto &v0 = polyhedron.vertices[face.x];
        const auto &v1 = polyhedron.vertices[face.y];
        const auto &v2 = polyhedron.vertices[face.z];

        const std::array<glm::vec3, 3> raw_edges = {v1 - v0, v2 - v1, v0 - v2};
        for (const auto &edge : raw_edges)
        {
            if (glm::dot(edge, edge) > EPSILON * EPSILON)
            {
                edges.push_back(glm::normalize(edge));
            }
        }
    }

    return edges;
}

std::pair<float, float> projectVerticesOnAxis(const std::vector<glm::vec3> &vertices, const glm::vec3 &axis)
{
    float min_proj = std::numeric_limits<float>::max();
    float max_proj = std::numeric_limits<float>::lowest();

    for (const auto &vertex : vertices)
    {
        const float projection = glm::dot(vertex, axis);
        min_proj = glm::min(min_proj, projection);
        max_proj = glm::max(max_proj, projection);
    }

    return {min_proj, max_proj};
}

Collider::ConvexPolyhedron fromAABB(const Collider::AABB &aabb)
{
    const auto v0 = aabb.center + glm::vec3(-aabb.half_size.x, -aabb.half_size.y, -aabb.half_size.z);
    const auto v1 = aabb.center + glm::vec3(aabb.half_size.x, -aabb.half_size.y, -aabb.half_size.z);
    const auto v2 = aabb.center + glm::vec3(-aabb.half_size.x, aabb.half_size.y, -aabb.half_size.z);
    const auto v3 = aabb.center + glm::vec3(aabb.half_size.x, aabb.half_size.y, -aabb.half_size.z);
    const auto v4 = aabb.center + glm::vec3(-aabb.half_size.x, -aabb.half_size.y, aabb.half_size.z);
    const auto v5 = aabb.center + glm::vec3(aabb.half_size.x, -aabb.half_size.y, aabb.half_size.z);
    const auto v6 = aabb.center + glm::vec3(-aabb.half_size.x, aabb.half_size.y, aabb.half_size.z);
    const auto v7 = aabb.center + glm::vec3(aabb.half_size.x, aabb.half_size.y, aabb.half_size.z);

    return {
        .vertices =
            {
                v0,
                v1,
                v2,
                v3,
                v4,
                v5,
                v6,
                v7,
            },
        .faces =
            {
                {0, 1, 3},
                {0, 3, 2}, // bottom (-z)
                {4, 6, 7},
                {4, 7, 5}, // top (+z)
                {0, 4, 5},
                {0, 5, 1}, // back (-y)
                {2, 3, 7},
                {2, 7, 6}, // front (+y)
                {0, 2, 6},
                {0, 6, 4}, // left (-x)
                {1, 5, 7},
                {1, 7, 3}, // right (+x)
            },
    };
}

bool collide(const Collider::ConvexPolyhedron &a, const Collider::ConvexPolyhedron &b)
{
    if (a.vertices.empty() || a.faces.empty() || b.vertices.empty() || b.faces.empty())
    {
        return false;
    }

    std::vector<glm::vec3> axes;
    axes.reserve(a.faces.size() + b.faces.size() + a.faces.size() * b.faces.size() * 9);

    appendFaceAxes(a, axes);
    appendFaceAxes(b, axes);

    const auto a_edges = collectEdgeDirections(a);
    const auto b_edges = collectEdgeDirections(b);
    for (const auto &edge_a : a_edges)
    {
        for (const auto &edge_b : b_edges)
        {
            const auto axis = glm::cross(edge_a, edge_b);
            if (glm::dot(axis, axis) > EPSILON * EPSILON)
            {
                axes.push_back(glm::normalize(axis));
            }
        }
    }

    for (const auto &axis : axes)
    {
        const auto [min_a, max_a] = projectVerticesOnAxis(a.vertices, axis);
        const auto [min_b, max_b] = projectVerticesOnAxis(b.vertices, axis);

        if (max_a < min_b - EPSILON || max_b < min_a - EPSILON)
        {
            return false;
        }
    }

    return true;
}

bool collide(const Collider::Sphere &sphere_a, const glm::mat4 &transform_a, const Collider::Sphere &sphere_b,
             const glm::mat4 &transform_b)
{
    const auto [center_a, radius_a] = transformSphere(sphere_a, transform_a);
    const auto [center_b, radius_b] = transformSphere(sphere_b, transform_b);
    return glm::dot(center_a - center_b, center_a - center_b) <= (radius_a + radius_b) * (radius_a + radius_b);
}

} // namespace

Collider::Collider(Type type, bool is_water)
    : type_(type), collision_resolution_mask_(1.0f), enabled_(true), is_water_(is_water)
{
}

void Collider::setCollisionResolutionMask(const glm::vec3 &mask)
{
    collision_resolution_mask_ = glm::max(mask, glm::vec3(0.0f));
}

const glm::vec3 &Collider::getCollisionResolutionMask() const
{
    return collision_resolution_mask_;
}

void Collider::disable()
{
    enabled_ = false;
}

bool Collider::isDisabled()
{
    return !enabled_;
}

bool Collider::collideWith(const Collider &other) const
{
    if (!collideWithAABB(other))
    {
        return false;
    }

    if (std::holds_alternative<AABB>(type_) && std::holds_alternative<AABB>(other.type_))
    {
        return true;
    }
    if (std::holds_alternative<Sphere>(type_) && std::holds_alternative<Sphere>(other.type_))
    {
        return collide(std::get<Sphere>(type_), transform_.lock()->resolve(), std::get<Sphere>(other.type_),
                       other.transform_.lock()->resolve());
    }
    if (std::holds_alternative<Sphere>(type_) && std::holds_alternative<AABB>(other.type_))
    {
        const auto [center, radius] = transformSphere(std::get<Sphere>(type_), transform_.lock()->resolve());
        return collideSphereAabb(center, radius, other.aabb_);
    }
    if (std::holds_alternative<AABB>(type_) && std::holds_alternative<Sphere>(other.type_))
    {
        const auto [center, radius] = transformSphere(std::get<Sphere>(other.type_), other.transform_.lock()->resolve());
        return collideSphereAabb(center, radius, aabb_);
    }
    if (std::holds_alternative<ConvexPolyhedron>(type_) && std::holds_alternative<AABB>(other.type_))
    {
        const auto self = transformPolyhedron(std::get<ConvexPolyhedron>(type_), transform_.lock()->resolve());
        return collide(self, fromAABB(other.aabb_));
    }
    if (std::holds_alternative<AABB>(type_) && std::holds_alternative<ConvexPolyhedron>(other.type_))
    {
        const auto other_poly =
            transformPolyhedron(std::get<ConvexPolyhedron>(other.type_), other.transform_.lock()->resolve());
        return collide(fromAABB(aabb_), other_poly);
    }
    if (std::holds_alternative<ConvexPolyhedron>(type_) && std::holds_alternative<ConvexPolyhedron>(other.type_))
    {
        const auto self = transformPolyhedron(std::get<ConvexPolyhedron>(type_), transform_.lock()->resolve());
        const auto other_poly =
            transformPolyhedron(std::get<ConvexPolyhedron>(other.type_), other.transform_.lock()->resolve());
        return collide(self, other_poly);
    }
    if (std::holds_alternative<Sphere>(type_) && std::holds_alternative<ConvexPolyhedron>(other.type_))
    {
        const auto [center, radius] = transformSphere(std::get<Sphere>(type_), transform_.lock()->resolve());
        const auto other_poly = transformPolyhedron(std::get<ConvexPolyhedron>(other.type_), other.transform_.lock()->resolve());
        return collideSpherePolyhedron(center, radius, other_poly);
    }
    if (std::holds_alternative<ConvexPolyhedron>(type_) && std::holds_alternative<Sphere>(other.type_))
    {
        const auto self = transformPolyhedron(std::get<ConvexPolyhedron>(type_), transform_.lock()->resolve());
        const auto [center, radius] = transformSphere(std::get<Sphere>(other.type_), other.transform_.lock()->resolve());
        return collideSpherePolyhedron(center, radius, self);
    }

    throw std::runtime_error("missing implementation for collision check");
}

bool Collider::collideWithAABB(const Collider &other) const
{
    float self_min_x = aabb_.center.x - aabb_.half_size.x;
    float self_max_x = aabb_.center.x + aabb_.half_size.x;
    float self_min_y = aabb_.center.y - aabb_.half_size.y;
    float self_max_y = aabb_.center.y + aabb_.half_size.y;
    float self_min_z = aabb_.center.z - aabb_.half_size.z;
    float self_max_z = aabb_.center.z + aabb_.half_size.z;

    float other_min_x = other.aabb_.center.x - other.aabb_.half_size.x;
    float other_max_x = other.aabb_.center.x + other.aabb_.half_size.x;
    float other_min_y = other.aabb_.center.y - other.aabb_.half_size.y;
    float other_max_y = other.aabb_.center.y + other.aabb_.half_size.y;
    float other_min_z = other.aabb_.center.z - other.aabb_.half_size.z;
    float other_max_z = other.aabb_.center.z + other.aabb_.half_size.z;

    return (self_min_x <= other_max_x && other_min_x <= self_max_x) &&
           (self_min_y <= other_max_y && other_min_y <= self_max_y) &&
           (self_min_z <= other_max_z && other_min_z <= self_max_z);
}

void Collider::addCollisionCallback(CollisionCallback callback)
{
    collision_callbacks_.push_back(callback);
}

bool Collider::callCollisionCallbacks(GameObjectId game_object_id) const
{
    bool should_be_detached = false;
    for (const auto &callback : collision_callbacks_)
    {
        if (callback(game_object_id))
            should_be_detached = true;
    }

    return should_be_detached;
}

void Collider::initialize()
{
    GET_COMPONENT(Transform, transform_, Collider);

    Physics::addCollider(std::dynamic_pointer_cast<Collider>(Component::shared_from_this()), is_water_);
}

void Collider::update(float delta_time)
{
    (void)delta_time;

    const auto transform = transform_.lock()->resolve();

    std::visit(
        [&](const auto &shape) {
            using Shape = std::decay_t<decltype(shape)>;

            if constexpr (std::is_same_v<Shape, AABB>)
            {
                const AABB &local_aabb = shape;

                glm::vec3 min;
                glm::vec3 max;
                for (uint8_t i = 0; i < 8; ++i)
                {
                    const float x = ((i & 1) ? 1.0f : -1.0f) * local_aabb.half_size.x;
                    const float y = ((i & 2) ? 1.0f : -1.0f) * local_aabb.half_size.y;
                    const float z = ((i & 4) ? 1.0f : -1.0f) * local_aabb.half_size.z;

                    const auto corner = glm::vec3(transform * glm::vec4(glm::vec3(x, y, z) + local_aabb.center, 1.0f));

                    if (i == 0)
                    {
                        min = corner;
                        max = corner;
                    }
                    else
                    {
                        min.x = glm::min(min.x, corner.x);
                        min.y = glm::min(min.y, corner.y);
                        min.z = glm::min(min.z, corner.z);
                        max.x = glm::max(max.x, corner.x);
                        max.y = glm::max(max.y, corner.y);
                        max.z = glm::max(max.z, corner.z);
                    }
                }

                aabb_.half_size = (max - min) * 0.5f;
                aabb_.center = (max + min) * 0.5f;
            }
            else if constexpr (std::is_same_v<Shape, ConvexPolyhedron>)
            {
                aabb_ = computeAabb(shape.vertices, transform);
            }
            else if constexpr (std::is_same_v<Shape, Sphere>)
            {
                const auto [center, radius] = transformSphere(shape, transform);
                aabb_.center = center;
                aabb_.half_size = radius * ONE;
            }
            else
            {
                throw std::runtime_error("missing update implementation for Collider type");
            }
        },
        type_);
}

bool Collider::render() const
{
    constexpr const Color AABB_COLOR = rgb(255, 0, 0);
    constexpr const Color CONVEX_POLYHEDRON_COLOR = rgb(0, 255, 0);
    constexpr const Color SPHERE_COLOR = rgb(0, 128, 255);
    constexpr const GLfloat LINE_WIDTH = 3.0f;
    constexpr const size_t SPHERE_SEGMENTS = 24;

    if (Singleton::debug)
    {
        GLenum previous_polygon_fill_mode[2];
        glGetIntegerv(GL_POLYGON_MODE, reinterpret_cast<GLint *>(previous_polygon_fill_mode));

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        std::visit(
            [&](const auto &shape) {
                using Shape = std::decay_t<decltype(shape)>;

                // collider shape
                if constexpr (std::is_same_v<Shape, AABB>)
                {
                    // pass: already rendered anyway
                }
                else if constexpr (std::is_same_v<Shape, ConvexPolyhedron>)
                {
                    const auto &polyhedron = shape;
                    if (polyhedron.vertices.empty() || polyhedron.faces.empty())
                    {
                        throw std::runtime_error("convex polyhedron collider needs vertices and faces");
                    }

                    constexpr const GLfloat material_ambient[] = {_v4(CONVEX_POLYHEDRON_COLOR)};
                    constexpr const GLfloat material_diffuse[] = {_v4(CONVEX_POLYHEDRON_COLOR)};

                    glMaterialfv(GL_FRONT, GL_AMBIENT, material_ambient);
                    glMaterialfv(GL_FRONT, GL_DIFFUSE, material_diffuse);

                    glLineWidth(LINE_WIDTH);
                    glBegin(GL_LINES);
                    for (const auto &face : polyhedron.faces)
                    {
                        const auto &v1 = polyhedron.vertices[face.x];
                        const auto &v2 = polyhedron.vertices[face.y];
                        const auto &v3 = polyhedron.vertices[face.z];

                        glVertex3f(_v3(v1));
                        glVertex3f(_v3(v2));
                        glVertex3f(_v3(v2));
                        glVertex3f(_v3(v3));
                        glVertex3f(_v3(v3));
                        glVertex3f(_v3(v1));
                    }
                    glEnd();
                }
                else if constexpr (std::is_same_v<Shape, Sphere>)
                {
                    const auto [center, radius] = transformSphere(shape, transform_.lock()->resolve());

                    constexpr const GLfloat material_ambient[] = {_v4(SPHERE_COLOR)};
                    constexpr const GLfloat material_diffuse[] = {_v4(SPHERE_COLOR)};

                    glPushMatrix();
                    glLoadIdentity();
                    Singleton::active_camera.lock()->bind();

                    glMaterialfv(GL_FRONT, GL_AMBIENT, material_ambient);
                    glMaterialfv(GL_FRONT, GL_DIFFUSE, material_diffuse);

                    glLineWidth(LINE_WIDTH);

                    const auto draw_ring = [&](const glm::vec3 &axis_x, const glm::vec3 &axis_y) {
                        glBegin(GL_LINE_LOOP);
                        for (size_t i = 0; i < SPHERE_SEGMENTS; ++i)
                        {
                            const float angle = glm::two_pi<float>() * static_cast<float>(i) /
                                                static_cast<float>(SPHERE_SEGMENTS);
                            const auto point = center + radius * (std::cos(angle) * axis_x + std::sin(angle) * axis_y);
                            glVertex3f(_v3(point));
                        }
                        glEnd();
                    };

                    draw_ring(X, Y);
                    draw_ring(Y, Z);
                    draw_ring(Z, X);

                    glPopMatrix();
                }
                else
                {
                    throw std::runtime_error("missing render implementation for Collider type");
                }
            },
            type_);

        // AABB
        {
            glPushMatrix();
            glLoadIdentity();
            Singleton::active_camera.lock()->bind();

            const auto v1 = glm::vec3(aabb_.half_size.x, aabb_.half_size.y, -aabb_.half_size.z) + aabb_.center;
            const auto v2 = glm::vec3(-aabb_.half_size.x, aabb_.half_size.y, -aabb_.half_size.z) + aabb_.center;
            const auto v3 = glm::vec3(-aabb_.half_size.x, -aabb_.half_size.y, -aabb_.half_size.z) + aabb_.center;
            const auto v4 = glm::vec3(aabb_.half_size.x, -aabb_.half_size.y, -aabb_.half_size.z) + aabb_.center;
            const auto v5 = glm::vec3(aabb_.half_size.x, aabb_.half_size.y, aabb_.half_size.z) + aabb_.center;
            const auto v6 = glm::vec3(-aabb_.half_size.x, aabb_.half_size.y, aabb_.half_size.z) + aabb_.center;
            const auto v7 = glm::vec3(-aabb_.half_size.x, -aabb_.half_size.y, aabb_.half_size.z) + aabb_.center;
            const auto v8 = glm::vec3(aabb_.half_size.x, -aabb_.half_size.y, aabb_.half_size.z) + aabb_.center;

            constexpr const GLfloat material_ambient[] = {_v4(AABB_COLOR)};
            constexpr const GLfloat material_diffuse[] = {_v4(AABB_COLOR)};

            glMaterialfv(GL_FRONT, GL_AMBIENT, material_ambient);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, material_diffuse);

            glLineWidth(LINE_WIDTH);
            glBegin(GL_LINES);
                glVertex3f(_v3(v1)); glVertex3f(_v3(v2));
                glVertex3f(_v3(v2)); glVertex3f(_v3(v3));
                glVertex3f(_v3(v3)); glVertex3f(_v3(v4));
                glVertex3f(_v3(v4)); glVertex3f(_v3(v1));

                glVertex3f(_v3(v5)); glVertex3f(_v3(v6));
                glVertex3f(_v3(v6)); glVertex3f(_v3(v7));
                glVertex3f(_v3(v7)); glVertex3f(_v3(v8));
                glVertex3f(_v3(v8)); glVertex3f(_v3(v5));

                glVertex3f(_v3(v1)); glVertex3f(_v3(v5));
                glVertex3f(_v3(v2)); glVertex3f(_v3(v6));
                glVertex3f(_v3(v3)); glVertex3f(_v3(v7));
                glVertex3f(_v3(v4)); glVertex3f(_v3(v8));
            glEnd();

            glPopMatrix();
        }

        glPolygonMode(GL_FRONT, previous_polygon_fill_mode[0]);
        glPolygonMode(GL_BACK, previous_polygon_fill_mode[1]);
    }

    return false;
}
