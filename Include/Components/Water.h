#pragma once

#include <Lib/OpenGL.h>

#include "Components/Component.h"

namespace component
{

class Water : public Component
{
  public:
    Water();

    bool render() const override;
  
  private:
    GLuint vertex_array_;
    GLuint vertex_buffer_;
    GLuint index_buffer_;
    GLsizei index_count_;
};

} // namespace component
