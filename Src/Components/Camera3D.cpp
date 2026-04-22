#include "Components/Camera3D.h"

#include <stdexcept>

#include <Lib/OpenGL.h>

#include "Events/EventQueue.h"
#include "Events/WindowResized.h"
#include "GameObject.h" // IWYU pragma: keep
#include "Singleton.h"
#include "Utils/Color.h"
#include "Utils/Constants.h"
#include "Utils/Math.h"
#include "Utils/Profiling.h"
#include "Utils/Random.h"

using namespace component;

Camera3D::Camera3D(Data data, const glm::vec3 &forward, bool display_effects)
    : display_effects_(display_effects), data_(data), forward_(forward)
{
    if (!static_initialized_)
    {
        EventQueue::registerCallback<event::WindowResized>(
            [](const event::WindowResized &event) { onViewportResize(event.width, event.height); });
        static_initialized_ = true;
    }
}

Camera3D::Camera3D(Perspective perspective, const glm::vec3 &forward, bool display_effects)
    : Camera3D(Data(perspective), forward, display_effects)
{
}

Camera3D::Camera3D(Orthographic orthographic, const glm::vec3 &forward, bool display_effects)
    : Camera3D(Data(orthographic), forward, display_effects)
{
}

void Camera3D::initialize()
{
    GET_COMPONENT(Transform, transform_, Camera3D);
}

bool Camera3D::render() const
{
    if (Singleton::debug)
    {
        constexpr const GLfloat MATERIAL_BLUE[] = {_v4(color::BLUE)};
        glMaterialfv(GL_FRONT, GL_AMBIENT, MATERIAL_BLUE);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, MATERIAL_BLUE);

        if (std::holds_alternative<Perspective>(data_))
        {
            const auto &perspective = std::get<Perspective>(data_);

            const auto forward = glm::normalize(forward_);
            const auto right = glm::normalize(glm::cross(forward, UP));
            const auto up = glm::normalize(glm::cross(right, forward));

            const auto near_center = forward * static_cast<float>(perspective.near);
            const auto near_height = 2.0f * static_cast<float>(perspective.near) *
                                     glm::tan(glm::radians(static_cast<float>(perspective.fov) * 0.5f));
            const auto near_width = near_height * viewport_width / viewport_height;

            const auto half_width = right * (near_width * 0.5f);
            const auto half_height = up * (near_height * 0.5f);

            const auto near_top_left = near_center - half_width + half_height;
            const auto near_top_right = near_center + half_width + half_height;
            const auto near_bottom_left = near_center - half_width - half_height;
            const auto near_bottom_right = near_center + half_width - half_height;

            glLineWidth(2.0f);

            glBegin(GL_LINES);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(_v3(near_top_left));

            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(_v3(near_top_right));

            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(_v3(near_bottom_left));

            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(_v3(near_bottom_right));

            glVertex3f(_v3(near_top_left));
            glVertex3f(_v3(near_top_right));

            glVertex3f(_v3(near_top_right));
            glVertex3f(_v3(near_bottom_right));

            glVertex3f(_v3(near_bottom_right));
            glVertex3f(_v3(near_bottom_left));

            glVertex3f(_v3(near_bottom_left));
            glVertex3f(_v3(near_top_left));
            glEnd();

            glPointSize(8.0f);

            constexpr const GLfloat MATERIAL_RED[] = {_v4(color::RED)};
            glMaterialfv(GL_FRONT, GL_AMBIENT, MATERIAL_RED);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, MATERIAL_RED);

            glBegin(GL_POINTS);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glEnd();
        }
    }

    return false;
}

void Camera3D::onViewportResize(uint32_t width, uint32_t height)
{
    viewport_width = static_cast<float>(width);
    viewport_height = static_cast<float>(height);
    aspect_ratio_ = static_cast<double>(width) / static_cast<double>(height);
}

void Camera3D::displayEffect(std::shared_ptr<resource::Texture> texture, Duration duration)
{
    effect_ = texture;
    effect_duration_ = duration;
    effect_start_time_ = now();
}

void Camera3D::shake(Duration duration)
{
    constexpr const float SHAKING_INTENSITY = 2.0f;

    shaking_duration_ = duration;
    shaking_start_ = now();
    shaking_offset_ = {SHAKING_INTENSITY, 0.0f};
    last_shake_ = now();
}

void Camera3D::updateEffect(float delta_time)
{
    (void)delta_time;

    constexpr const float SHAKING_SPREAD_ANGLE = glm::radians(60.0f);

    const auto instant_now = now();

    if (instant_now >= effect_start_time_ + effect_duration_)
    {
        effect_ = nullptr;
    }

    if (instant_now < shaking_start_ + shaking_duration_)
    {
        const auto elapsed = instant_now - shaking_start_;
        const double duration = static_cast<double>(shaking_duration_.count());
        const float t = static_cast<float>(
            glm::clamp((duration > 0.0) ? (static_cast<double>(elapsed.count()) / duration) : 1.0, 0.0, 1.0));
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
    if (effect_ == nullptr)
        return;

    if (!display_effects_)
        return;

    PUSH_CLEAR_STATE();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    const GLboolean depth_test_was_enabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean lighting_was_enabled = glIsEnabled(GL_LIGHTING);
    const GLboolean cull_face_was_enabled = glIsEnabled(GL_CULL_FACE);
    GLboolean depth_write_was_enabled = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depth_write_was_enabled);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);

    constexpr const glm::vec2 TOP_RIGHT = {1.0f, -1.0f};
    constexpr const glm::vec2 TOP_LEFT = {-1.0f, -1.0f};
    constexpr const glm::vec2 BOTTOM_LEFT = {-1.0f, 1.0f};
    constexpr const glm::vec2 BOTTOM_RIGHT = {1.0f, 1.0f};

    const auto ambient = glm::vec4(glm::vec3(color::WHITE) * 0.5f, color::WHITE.w);
    GLfloat ambient_color[] = {_v4(ambient)};
    GLfloat diffuse_color[] = {_v4(color::WHITE)};
    GLfloat specular_color[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient_color);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse_color);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular_color);

    glEnable(GL_TEXTURE_2D);
    effect_->bind(GL_TEXTURE0);
    const auto elapsed = now() - effect_start_time_;
    const double duration = static_cast<double>(effect_duration_.count());
    const double t_unclamped = (duration > 0.0) ? (static_cast<double>(elapsed.count()) / duration) : 1.0;
    const float t = static_cast<float>(t_unclamped < 0.0 ? 0.0 : (t_unclamped > 1.0 ? 1.0 : t_unclamped));
    const float alpha = 1.0f - t * t;
    glColor4f(1.0f, 1.0f, 1.0f, alpha);

    glBegin(GL_QUADS);
        glTexCoord2f(1.0f, 0.0f); glVertex2f(_v2(TOP_RIGHT));
        glTexCoord2f(0.0f, 0.0f); glVertex2f(_v2(TOP_LEFT));
        glTexCoord2f(0.0f, 1.0f); glVertex2f(_v2(BOTTOM_LEFT));
        glTexCoord2f(1.0f, 1.0f); glVertex2f(_v2(BOTTOM_RIGHT));
    glEnd();

    effect_->unbind(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);

    if (cull_face_was_enabled)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);

    if (lighting_was_enabled)
        glEnable(GL_LIGHTING);
    else
        glDisable(GL_LIGHTING);

    glDepthMask(depth_write_was_enabled);
    if (depth_test_was_enabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    POP_CLEAR_STATE();
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
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    if (std::holds_alternative<Perspective>(data_))
    {
        const auto &perspective = std::get<Perspective>(data_);
        gluPerspective(perspective.fov, aspect_ratio_, perspective.near, perspective.far);
    }
    else if (std::holds_alternative<Orthographic>(data_))
    {
        const auto &orthographic = std::get<Orthographic>(data_);

        double left = -orthographic.scale * aspect_ratio_;
        double right = orthographic.scale * aspect_ratio_;
        double bottom = -orthographic.scale;
        double top = orthographic.scale;

        glOrtho(left, right, bottom, top, orthographic.near, orthographic.far);
    }
    else
    {
        throw std::runtime_error("Camera3D type not implementd");
    }

    const auto forward_world = forward();
    const auto right_world = glm::normalize(glm::cross(forward_world, UP));
    const auto fallback_right_world = glm::normalize(glm::cross(forward_world, EAST));
    const auto plane_right = (glm::length(right_world) > EPSILON) ? right_world : fallback_right_world;
    const auto plane_up = glm::normalize(glm::cross(plane_right, forward_world));

    const glm::vec3 offset = display_effects_ ? (shaking_offset_.x * plane_right + shaking_offset_.y * plane_up) : ZERO;
    const auto eye = getPosition() + offset;
    const auto look_at = eye + forward_world;

    // View Matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(_dv3(eye), _dv3(look_at), _dv3(UP));
}

glm::vec3 Camera3D::screenToWorld(const glm::vec2 &screen_position) const
{
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
