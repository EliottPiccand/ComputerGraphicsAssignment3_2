#pragma once

#include <Lib/OpenGL.h>
#include <Lib/glm.h>

using Color = glm::vec4;

#define rgba(r, g, b, a) Color{r##.0f / 255.0f, g##.0f / 255.0f, b##.0f / 255.0f, static_cast<float>(a)}
#define rgb(r, g, b) rgba(r, g, b, 1)

namespace color
{

constexpr const Color WHITE = rgb(255, 255, 255);
constexpr const Color BLACK = rgb(0, 0, 0);
constexpr const Color TRANSPARENT = rgba(0, 0, 0, 0);
constexpr const Color RED = rgb(255, 0, 0);
constexpr const Color GREEN = rgb(0, 255, 0);
constexpr const Color BLUE = rgb(0, 0, 255);

constexpr const GLfloat MATERIAL_RED[] = {_v4(color::RED)};
constexpr const GLfloat MATERIAL_GREEN[] = {_v4(color::GREEN)};
constexpr const GLfloat MATERIAL_BLUE[] = {_v4(color::BLUE)};
constexpr const GLfloat MATERIAL_BLACK[] = {_v4(color::BLACK)};

} // namespace color
