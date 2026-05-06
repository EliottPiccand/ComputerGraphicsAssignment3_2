#include "Mesh/Vertex/VertexWater.h"

#include <cstddef>

#include <Lib/OpenGL.h>

void VertexWater::setupVertexArray()
{
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexWater),
                          reinterpret_cast<const void *>(offsetof(VertexWater, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VertexWater),
                          reinterpret_cast<const void *>(offsetof(VertexWater, uv)));
    glEnableVertexAttribArray(1);
}
