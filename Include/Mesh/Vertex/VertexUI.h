#pragma once

#include <Lib/glm.h>

struct VertexUI
{
    glm::vec2 position;
    glm::vec2 uv;

    static void setupVertexArray();
};
