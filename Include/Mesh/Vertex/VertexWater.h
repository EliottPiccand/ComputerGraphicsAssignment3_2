#pragma once

#include <Lib/glm.h>

struct VertexWater
{
    glm::vec3 position;
    glm::vec4 uv;

    static void setupVertexArray();
};
