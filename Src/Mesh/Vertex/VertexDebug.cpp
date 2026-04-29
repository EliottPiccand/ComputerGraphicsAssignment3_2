#include "Mesh/Vertex/VertexDebug.h"

#include <cstddef>

#include <Lib/OpenGL.h>

void VertexDebug::setupVertexArray()
{
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexDebug),
                          reinterpret_cast<const void *>(offsetof(VertexDebug, position)));
    glEnableVertexAttribArray(0);
}
