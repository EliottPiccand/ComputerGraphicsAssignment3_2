#include "Components/Camera3D.h"

#include <stdexcept>

#include <Lib/OpenGL.h>

#include "Events/EventQueue.h"
#include "Events/WindowResized.h"
#include "GameObject.h" // IWYU pragma: keep
#include "Resources/ResourceLoader.h"
#include "Utils/Constants.h"
#include "Utils/Math.h"
#include "Utils/Profiling.h"
#include "Utils/Random.h"


using namespace component;

Camera3D::Camera3D(Data data, const glm::vec3 &forward, bool display_effects)
    : display_effects_(display_effects), data_(data), forward_(forward)
{
}

Camera3D::Camera3D(Perspective perspective, const glm::vec3 &forward, bool display_effects)
    : Camera3D(Data(perspective), forward, display_effects)
{
}

Camera3D::Camera3D(Orthographic orthographic, const glm::vec3 &forward, bool display_effects)
    : Camera3D(Data(orthographic), forward, display_effects)
{
}

void Camera3D::initialize(std::vector<std::weak_ptr<resource::Shader>> shaders)
{
    ProfileScope;

    EventQueue::registerCallback<event::WindowResized>(
        [](const event::WindowResized &event) { onViewportResize(event.width, event.height); });

    shaders_ = shaders;
}

void Camera3D::initialize()
{
    ProfileScope;

    GET_COMPONENT(Transform, transform_, Camera3D);
}

void Camera3D::onViewportResize(uint32_t width, uint32_t height)
{
    viewport_width = static_cast<float>(width);
    viewport_height = static_cast<float>(height);
    aspect_ratio_ = static_cast<double>(width) / static_cast<double>(height);
}

void Camera3D::displayEffect(std::shared_ptr<resource::Texture> texture, Duration duration)
{
    effect_ = {
        {
            0,
            {
                .albedo_texture = texture,
            },
        },
    };
    effect_duration_ = duration;
    effect_start_time_ = Time::now();
}

void Camera3D::shake(Duration duration)
{
    constexpr const float SHAKING_INTENSITY = 2.0f;

    shaking_duration_ = duration;
    shaking_start_ = Time::now();
    shaking_offset_ = {SHAKING_INTENSITY, 0.0f};
    last_shake_ = Time::now();
}

void Camera3D::updateEffect()
{
    ProfileScope;

    constexpr const float SHAKING_SPREAD_ANGLE = glm::radians(60.0f);

    const auto instant_now = Time::now();

    if (instant_now >= effect_start_time_ + effect_duration_)
    {
        effect_ = {};
    }

    if (instant_now < shaking_start_ + shaking_duration_)
    {
        const auto elapsed = instant_now - shaking_start_;
        const double duration = static_cast<double>(shaking_duration_.toSeconds());
        const float t = static_cast<float>(
            glm::clamp((duration > 0.0) ? (static_cast<double>(elapsed.toSeconds()) / duration) : 1.0, 0.0, 1.0));
        const float alpha = 1.0f - t * t;

        shaking_offset_ =
            glm::rotate(shaking_offset_,
                        glm::radians(180.0f + Random::random(-SHAKING_SPREAD_ANGLE, SHAKING_SPREAD_ANGLE))) *
            alpha;
        last_shake_ = instant_now;
    }
}

void Camera3D::renderEffect() const
{
    ProfileScope;
    ProfileScopeGPU("Camera3D::renderEffect");

    static std::weak_ptr model = ResourceLoader::get<resource::Model>("Effect");
    static std::weak_ptr weak_shader = ResourceLoader::get<resource::Shader>("UI");

    if (effect_.size() == 0)
        return;

    if (!display_effects_)
        return;

    SCOPE_DISABLE(GL_DEPTH_TEST);
    SCOPE_DISABLE(GL_CULL_FACE);

    auto shader = weak_shader.lock();
    shader->bind();
    shader->setUniform("u_Model", glm::mat3(glm::scale(glm::mat4(1.0f), {2.0f, 2.0f, 1.0f})));
    shader->setUniform("u_Alpha", std::sqrt(std::sqrt(1.0f - (Time::now() - effect_start_time_).toSeconds() /
                                                                 effect_duration_.toSeconds())));
    effect_model_.lock()->draw(shader, effect_);
}

glm::vec3 Camera3D::getPosition() const
{
    return glm::vec3(transform_.lock()->resolve()[3]);
}

void Camera3D::bind() const
{
    ProfileScope;
    ProfileScopeGPU("Camera3D::bind");

    // Projection Matrix
    glm::mat4 projection;

    if (std::holds_alternative<Perspective>(data_))
    {
        const auto &perspective = std::get<Perspective>(data_);
        projection = glm::perspective(perspective.fov, aspect_ratio_, perspective.near, perspective.far);
    }
    else if (std::holds_alternative<Orthographic>(data_))
    {
        const auto &orthographic = std::get<Orthographic>(data_);

        double left = -orthographic.scale * aspect_ratio_;
        double right = orthographic.scale * aspect_ratio_;
        double bottom = -orthographic.scale;
        double top = orthographic.scale;

        projection = glm::ortho(left, right, bottom, top, orthographic.near, orthographic.far);
    }
    else
    {
        throw std::runtime_error("Camera3D type not implementd");
    }

    // View Matrix
    const auto forward_world = forward();
    const auto right_world = glm::normalize(glm::cross(forward_world, UP));
    const auto fallback_right_world = glm::normalize(glm::cross(forward_world, EAST));
    const auto plane_right = (glm::length(right_world) > EPSILON) ? right_world : fallback_right_world;
    const auto plane_up = glm::normalize(glm::cross(plane_right, forward_world));

    const glm::vec3 offset = display_effects_ ? (shaking_offset_.x * plane_right + shaking_offset_.y * plane_up) : ZERO;
    const auto eye = getPosition() + offset;
    const auto look_at = eye + forward_world;

    const glm::mat4 view = glm::lookAt(eye, look_at, UP);

    // Bind
    for (auto weak_shader : shaders_)
    {
        auto shader = weak_shader.lock();
        shader->bind();
        shader->setUniform("u_Projection", projection);
        shader->setUniform("u_View", view);
        shader->setUniform("u_CameraPosition", eye);

        shader->setUniform("u_WorldEast", EAST);
        shader->setUniform("u_WorldNorth", NORTH);
        shader->setUniform("u_WorldUp", UP);
    }
}

glm::vec3 Camera3D::screenToWorld(const glm::vec2 &screen_position) const
{
    ProfileScope;

    glm::mat4 projection;
    if (std::holds_alternative<Orthographic>(data_))
    {
        const auto &orthographic = std::get<Orthographic>(data_);

        double left = -orthographic.scale * aspect_ratio_;
        double right = orthographic.scale * aspect_ratio_;
        double bottom = -orthographic.scale;
        double top = orthographic.scale;

        projection = glm::ortho(left, right, bottom, top, orthographic.near, orthographic.far);
    }
    else if (std::holds_alternative<Perspective>(data_))
    {
        const auto &perspective = std::get<Perspective>(data_);
        projection = glm::perspective(glm::radians(perspective.fov), aspect_ratio_, perspective.near, perspective.far);
    }
    else
    {
        throw std::runtime_error("missing Camera3D::screenToWorld implementation for data alternative");
    }

    const auto transform = transform_.lock()->resolve();
    const auto eye = glm::vec3(transform[3]);
    const auto look_at = eye + glm::vec3(transform * glm::vec4(forward_, 0.0f));
    auto view = glm::lookAt(eye, look_at, UP);

    glm::vec4 screen_ndc_position = {
        (2.0f * screen_position.x) / viewport_width - 1.0f,
        1.0f - (2.0f * screen_position.y) / viewport_height,
        -1.0f,
        1.0f,
    };

    glm::vec4 world_point = glm::inverse(projection * view) * screen_ndc_position;
    world_point /= world_point.w;
    return world_point;
}

glm::vec3 Camera3D::forward() const
{
    return glm::normalize(glm::vec3(transform_.lock()->resolve() * glm::vec4(forward_, 0.0f)));
}

void Camera3D::lookToward(const glm::vec3 &forward)
{
    const auto transform = transform_.lock()->resolve();
    const auto local_forward = glm::inverse(transform) * glm::vec4(forward, 0.0f);
    forward_ = glm::normalize(glm::vec3(local_forward));
}
