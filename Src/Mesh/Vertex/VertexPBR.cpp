#include "Mesh/Vertex/VertexPBR.h"


#include <Lib/OpenGL.h>

void VertexPBR::setupVertexArray()
{
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPBR),
                          reinterpret_cast<const void *>(offsetof(VertexPBR, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPBR),
                          reinterpret_cast<const void *>(offsetof(VertexPBR, normal)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexPBR),
                          reinterpret_cast<const void *>(offsetof(VertexPBR, uv)));
    glEnableVertexAttribArray(2);
}
