#pragma once

#include <memory>

#include "Components/Component.h"
#include "Resources/Texture.h"
#include "Utils/MeshPrimitives.h"

namespace component
{

class Flag : public Component
{
  public:
    Flag(std::shared_ptr<resource::Texture> texture);

    static void createMesh();
    static void updateFlapping(float delta_time);
    bool render() const override;

  private:
    static inline Mesh mesh_;
    static inline GLuint vertex_array_;
    static inline GLuint vertex_buffer_;
    static inline GLuint index_buffer_;
    static inline GLsizei index_count_;

    std::shared_ptr<resource::Texture> texture_;
};

} // namespace component
