#pragma once

#include <memory>

#include <Lib/OpenGL.h>
#include <Lib/glm.h>

#include "Components/Component.h"
#include "Resources/Shader.h"
#include "Utils/Color.h"

struct DirectionalLight
{
    glm::vec3 direction;
    glm::vec3 color;
};

namespace component
{

class DirectionalLight : public Component
{
  public:
    static inline constexpr const float AMBIENT_INTENSITY = 0.6f;
    static inline constexpr const Color AMBIENT_COLOR = Color(AMBIENT_INTENSITY * glm::vec3(color::SKY), 1.0f);
    static inline constexpr const size_t MAX_DIRECTIONAL_LIGHTS = 16;

    /// direction must be normalized
    DirectionalLight(glm::vec3 direction, Color color, float intensity);
    ~DirectionalLight() override;

    static void initialize(std::vector<std::shared_ptr<resource::Shader>> shaders);

    static void beginRender();
    static void endRender();
    void render(glm::mat4 &transform) const override;

  private:
    static inline std::vector<std::weak_ptr<resource::Shader>> shaders_;
    static inline size_t lights_drawn_on_this_frame_;

    /// normalized
    glm::vec3 direction_;
    Color color_;
    float intensity_;
};

} // namespace component
