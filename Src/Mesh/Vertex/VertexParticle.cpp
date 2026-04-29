#include "Mesh/Vertex/VertexParticle.h"

#include <cstddef>

#include <Lib/OpenGL.h>

void VertexParticle::setupVertexArray()
{
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexParticle),
                          reinterpret_cast<const void *>(offsetof(VertexParticle, position)));
    glEnableVertexAttribArray(0);
}
