#include "Components/Transform.h"

#include <Lib/OpenGL.h>

#include "GameObject.h"
#include "Utils/Constants.h"
#include "Utils/Profiling.h"

using namespace component;

Transform::Transform(const glm::vec3 &position, const glm::vec3 &rotation, const glm::vec3 &scale)
    : position_(position), rotation_(rotation), scale_(scale)
{
}
Transform::Transform(const glm::vec3 &position, const glm::vec3 &rotation)
    : Transform(position, rotation, {1.0f, 1.0f, 1.0f})
{
}
Transform::Transform(const glm::vec3 &position) : Transform(position, {0.0f, 0.0f, 0.0f})
{
}
Transform::Transform() : Transform({0.0f, 0.0f, 0.0f})
{
}

glm::mat4 Transform::resolve() const
{
    ProfileScope;

    glm::mat4 transform;
    const auto ownerParentOpt = owner_.lock()->getParent();

    if (ownerParentOpt.has_value())
    {
        const auto previousTransformOpt = ownerParentOpt.value()->findFirstComponentInParents<Transform>();
        if (previousTransformOpt.has_value())
        {
            transform = previousTransformOpt.value()->resolve();
        }
        else
        {
            transform = glm::mat4(1.0f);
        }
    }
    else
    {
        transform = glm::mat4(1.0f);
    }

    transform = glm::translate(transform, position_);
    transform = transform * glm::mat4_cast(rotation_);
    transform = glm::scale(transform, scale_);

    return transform;
}

void Transform::translate(const glm::vec3 &by)
{
    position_ += by;
}

void Transform::rotate(const float angle, const glm::vec3 &axis)
{
    rotation_ = glm::angleAxis(angle, axis) * rotation_;
}

void Transform::setPosition(const glm::vec3 &position)
{
    position_ = position;
}

void Transform::setRotation(const glm::quat &rotation)
{
    rotation_ = rotation;
}

void Transform::setScale(const glm::vec3 &scale)
{
    scale_ = scale;
}

glm::vec3 Transform::getPosition() const
{
    return position_;
}

glm::quat Transform::getRotation() const
{
    return rotation_;
}

glm::vec3 Transform::getScale() const
{
    return scale_;
}

void Transform::pointToward(const glm::vec3 &direction)
{
    constexpr const float ROTATION_EPSILON = 1e-5f;

    const auto target_forward = glm::normalize(direction);
    const auto current_forward = glm::normalize(rotation_ * MODEL_FORWARD);
    const auto rotation_axis = glm::cross(current_forward, target_forward);
    const auto axis_length = glm::length(rotation_axis);

    if (axis_length < ROTATION_EPSILON)
    {
        if (glm::dot(current_forward, target_forward) < 0.0f)
        {
            const auto fallback_axis = glm::normalize(rotation_ * MODEL_RIGHT);
            rotation_ = glm::angleAxis(glm::pi<float>(), fallback_axis) * rotation_;
        }
        return;
    }

    const auto rotation_angle = std::atan2(axis_length, glm::dot(current_forward, target_forward));
    rotation_ = glm::normalize(glm::angleAxis(rotation_angle, rotation_axis / axis_length) * rotation_);
}

bool Transform::render() const
{
    ProfileScope;
    ProfileScopeGPU("Transform::render");

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();

    glm::mat4 rotation_matrix = glm::mat4_cast(rotation_);

    glTranslatef(_v3(position_));
    glMultMatrixf(glm::value_ptr(rotation_matrix));
    glScalef(_v3(scale_));

    return true;
}
