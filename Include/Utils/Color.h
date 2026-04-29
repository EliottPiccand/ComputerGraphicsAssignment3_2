#pragma once

#include <Lib/OpenGL.h>
#include <Lib/glm.h>

using Color = glm::vec4;

#define rgba(r, g, b, a)                                                                                               \
    Color                                                                                                              \
    {                                                                                                                  \
        r##.0f / 255.0f, g##.0f / 255.0f, b##.0f / 255.0f, static_cast<float>(a)                                       \
    }
#define rgb(r, g, b) rgba(r, g, b, 1)

namespace color
{

constexpr const Color WHITE = rgb(255, 255, 255);
constexpr const Color BLACK = rgb(0, 0, 0);
constexpr const Color TRANSPARENT = rgba(0, 0, 0, 0);
constexpr const Color RED = rgb(255, 0, 0);
constexpr const Color GREEN = rgb(0, 255, 0);
constexpr const Color BLUE = rgb(0, 0, 255);
constexpr const Color YELLOW = rgb(255, 255, 0);

} // namespace color
