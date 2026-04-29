#include "Mesh/Vertex/VertexUI.h"

#include <cstddef>

#include <Lib/OpenGL.h>

void VertexUI::setupVertexArray()
{
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexUI),
                          reinterpret_cast<const void *>(offsetof(VertexUI, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexUI),
                          reinterpret_cast<const void *>(offsetof(VertexUI, uv)));
    glEnableVertexAttribArray(1);
}
