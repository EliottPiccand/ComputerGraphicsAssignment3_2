#pragma once

#include <Lib/glm.h>

constexpr const glm::vec3 X = {1.0f, 0.0f, 0.0f};
constexpr const glm::vec3 Y = {0.0f, 1.0f, 0.0f};
constexpr const glm::vec3 Z = {0.0f, 0.0f, 1.0f};

constexpr const glm::vec3 NORTH = Y;
constexpr const glm::vec3 EAST = X;
constexpr const glm::vec3 UP = Z;
constexpr const glm::vec3 DOWN = -UP;
constexpr const glm::vec3 SOUTH = -NORTH;
constexpr const glm::vec3 WEST = -EAST;

constexpr const glm::vec3 MODEL_FORWARD = Y;
constexpr const glm::vec3 MODEL_RIGHT = X;
constexpr const glm::vec3 MODEL_UP = Z;
constexpr const glm::vec3 MODEL_BACKWARD = -MODEL_FORWARD;
constexpr const glm::vec3 MODEL_LEFT = -MODEL_RIGHT;
constexpr const glm::vec3 MODEL_DOWN = -MODEL_UP;

constexpr const float WORLD_WIDTH = 160.0f; // m

constexpr const float GRAVITY = 9.81f; // m / s^2

/// c.f. Ballistic.pdf
constexpr const float MAX_SHOOTING_INITIAL_HEIGHT = 5.0f;                 // m
inline const float MAX_SHOOTING_DISTANCE = glm::sqrt(2.0f) * WORLD_WIDTH; // m
inline const float INITIAL_CANNON_BALL_VELOCITY = glm::sqrt(
    GRAVITY * (MAX_SHOOTING_INITIAL_HEIGHT + glm::sqrt(MAX_SHOOTING_INITIAL_HEIGHT * MAX_SHOOTING_INITIAL_HEIGHT +
                                                       MAX_SHOOTING_DISTANCE * MAX_SHOOTING_DISTANCE))); // m/s
