#pragma once

#include <Lib/OpenGL.h>
#include <Lib/glm.h>

#include "Components/Component.h"

namespace component
{

class Water : public Component
{
  public:
    Water();

    void render(glm::mat4 &transform) const override;

  private:
    GLuint vertex_array_;
    GLuint vertex_buffer_;
    GLuint index_buffer_;
    GLsizei index_count_;
};

} // namespace component
