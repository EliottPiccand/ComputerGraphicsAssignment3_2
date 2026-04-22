#include "Components/FreeViewControls.h"

#include <variant>

#include <Lib/OpenGL.h>
#include <Lib/glm.h>

#include "Components/Camera3D.h"
#include "GameObject.h" // IWYU pragma: keep
#include "Input.h"
#include "Singleton.h"
#include "Utils/Color.h"
#include "Utils/Constants.h"
#include "Utils/Math.h"

using namespace component;

FreeViewControls::FreeViewControls()
{
    Input::bindKey(Input::Action::FreeViewForward, GLFW_KEY_W);
    Input::bindKey(Input::Action::FreeViewLeft, GLFW_KEY_A);
    Input::bindKey(Input::Action::FreeViewBackward, GLFW_KEY_S);
    Input::bindKey(Input::Action::FreeViewRight, GLFW_KEY_D);
    Input::bindKey(Input::Action::FreeViewUp, GLFW_KEY_SPACE);
    Input::bindKey(Input::Action::FreeViewDown, GLFW_KEY_LEFT_SHIFT);
}

void FreeViewControls::update(float delta_time)
{
    constexpr const float SPEED = 6.0f; // m/s
    constexpr const float VERTICAL_SENSITIVITY = 0.3f;
    constexpr const float HORIZONTAL_SENSITIVITY = 0.3f;
    constexpr const float MAX_PITCH = glm::radians(89.9f);
    constexpr const float MIN_PITCH = glm::radians(-89.9f);

    if (!active)
    {
        return;
    }

    const auto &camera = Singleton::active_camera.lock();
    if (std::holds_alternative<Camera3D::Perspective>(camera->data_))
    {
        auto camera_direction = camera->forward_;
        auto forward = glm::normalize(camera_direction - UP * glm::dot(camera_direction, UP));
        auto right = glm::cross(forward, UP);

        // rotation
        auto mouse_delta = Input::getMouseDelta();
        if (glm::length(mouse_delta) > EPSILON)
        {
            float delta_pitch = mouse_delta.y * delta_time * VERTICAL_SENSITIVITY;
            float pitch = glm::acos(glm::dot(camera_direction, forward)) * glm::sign(glm::dot(camera_direction, UP));
            delta_pitch = glm::clamp(delta_pitch, MIN_PITCH - pitch, MAX_PITCH - pitch);

            camera_direction = glm::rotate(camera_direction, delta_pitch, right);
            camera_direction = glm::rotate(camera_direction, mouse_delta.x * delta_time * HORIZONTAL_SENSITIVITY, UP);

            camera->forward_ = camera_direction;

            forward =
                glm::normalize(glm::dot(camera_direction, NORTH) * NORTH + glm::dot(camera_direction, EAST) * EAST);
            right = glm::cross(forward, UP);
        }

        // motion
        glm::vec3 motion{};

        if (Input::isPressed(Input::Action::FreeViewForward))
        {
            motion += forward;
        }
        if (Input::isPressed(Input::Action::FreeViewLeft))
        {
            motion -= right;
        }
        if (Input::isPressed(Input::Action::FreeViewBackward))
        {
            motion -= forward;
        }
        if (Input::isPressed(Input::Action::FreeViewRight))
        {
            motion += right;
        }

        if (glm::length(motion) > EPSILON)
        {
            motion = glm::normalize(motion);
        }

        if (Input::isPressed(Input::Action::FreeViewUp))
        {
            motion += UP;
        }
        if (Input::isPressed(Input::Action::FreeViewDown))
        {
            motion -= UP;
        }

        if (glm::length(motion) > EPSILON)
        {
            motion *= SPEED * delta_time;

            camera->transform_.lock()->translate(motion);
        }
    }
}

bool FreeViewControls::render() const
{
    constexpr const GLfloat LENGTH = 0.002f;

    if (Singleton::debug && Singleton::view == View::FreeCamera)
    {
        PUSH_CLEAR_STATE();

        const auto camera = Singleton::active_camera.lock();
        camera->bind();

        const auto perspective = std::get<Camera3D::Perspective>(camera->data_);
        const auto position =
            camera->getPosition() + camera->forward() * (static_cast<float>(perspective.near) + LENGTH);

        glMatrixMode(GL_MODELVIEW);
        glTranslatef(_v3(position));

        glLineWidth(2.0f);

        constexpr const auto EAST_LINE_END = EAST * LENGTH;
        constexpr const auto NORTH_LINE_END = NORTH * LENGTH;
        constexpr const auto UP_LINE_END = UP * LENGTH;

        glBegin(GL_LINES);
            glMaterialfv(GL_FRONT, GL_AMBIENT, color::MATERIAL_RED);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, color::MATERIAL_RED);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(_v3(EAST_LINE_END));

            glMaterialfv(GL_FRONT, GL_AMBIENT, color::MATERIAL_GREEN);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, color::MATERIAL_GREEN);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(_v3(NORTH_LINE_END));            

            glMaterialfv(GL_FRONT, GL_AMBIENT, color::MATERIAL_BLUE);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, color::MATERIAL_BLUE);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(_v3(UP_LINE_END));
        glEnd();

        POP_CLEAR_STATE();
    }

    return false;
}
