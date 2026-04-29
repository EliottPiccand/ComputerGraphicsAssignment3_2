#pragma once

#include <Lib/glm.h>

#include "Utils/Constants.h"

constexpr const float EPSILON = 1e-5f;
constexpr const glm::vec3 ZERO = {};
constexpr const glm::vec3 ONE = {1.0f, 1.0f, 1.0f};

[[nodiscard]] glm::quat twistAroundAxis(const glm::quat &q, const glm::vec3 &axis);

/// axis must be normalized
[[nodiscard]] float angleAroundAxis(const glm::quat &q, const glm::vec3 &axis);

[[nodiscard]] glm::vec3 getForwardVector(const glm::quat &orientation, const glm::vec3 &local_forward = MODEL_FORWARD);
