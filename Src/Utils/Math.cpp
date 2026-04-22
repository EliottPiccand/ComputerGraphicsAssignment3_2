#include "Utils/Math.h"

glm::quat twistAroundAxis(const glm::quat &q, const glm::vec3 &axis)
{
    glm::vec3 projection = axis * glm::dot(glm::vec3(q.x, q.y, q.z), axis);
    glm::quat twist(q.w, projection.x, projection.y, projection.z);
    float length = glm::length(twist);
    if (length < EPSILON)
        return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

    return twist / length;
}

float angleAroundAxis(const glm::quat &q, const glm::vec3 &axis)
{
    glm::quat twist = twistAroundAxis(q, axis);
    return 2.0f * std::atan2(glm::dot(glm::vec3(twist.x, twist.y, twist.z), axis), twist.w);
}

glm::vec3 getForwardVector(const glm::quat &orientation, const glm::vec3 &local_forward)
{
    return orientation * local_forward;
}
