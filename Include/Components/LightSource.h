#pragma once

#include <unordered_set>

#include <Lib/OpenGL.h>
#include <Lib/glm.h>

#include "Components/Component.h"
#include "Utils/Color.h"

namespace component
{

class LightSource : public Component
{
  public:
    static inline constexpr const Color AMBIENT_COLOR = color::WHITE;

    LightSource(Color ambient, Color diffuse);
    ~LightSource() override;

    bool render() const override;

  private:
    static inline std::unordered_set<GLenum> available_lights_ = {
        GL_LIGHT0, GL_LIGHT1, GL_LIGHT2, GL_LIGHT3, GL_LIGHT4, GL_LIGHT5, GL_LIGHT6, GL_LIGHT7,
    };

    GLenum light_id_;

    Color ambient_;
    Color diffuse_;
};

} // namespace component
