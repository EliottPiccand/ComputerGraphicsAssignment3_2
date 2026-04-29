#pragma once

#include <concepts>

template <typename T>
concept Vertex = requires {
    { T::setupVertexArray() } -> std::same_as<void>;
};
