#pragma once

#include <cstdint>
#include <memory>
#include <variant>

#include <Lib/glm.h>

#include "Components/Component.h"
#include "Components/Transform.h"
#include "Resources/Texture.h"
#include "Utils/Time.h"

namespace component
{

class FreeViewControls;

class Camera3D : public Component
{
  public:
    struct Perspective
    {
        double fov;
        double near;
        double far;
    };

    struct Orthographic
    {
        double scale;
        double near;
        double far;
    };
    using Data = std::variant<Perspective, Orthographic>;

    /// forward: local coordinates
    Camera3D(Perspective perspective, const glm::vec3 &forward, bool display_effects = true);
    /// forward: local coordinates
    Camera3D(Orthographic orthographic, const glm::vec3 &forward, bool display_effects = true);

    void initialize() override;
    bool render() const override;

    static void shake(Duration duration);
    static void displayEffect(std::shared_ptr<resource::Texture> texture, Duration duration);
    static void updateEffect(float delta_time);
    void renderEffect() const;

    static void onViewportResize(uint32_t width, uint32_t height);
    void bind() const;

    [[nodiscard]] glm::vec3 screenToWorld(const glm::vec2 &screen_position) const;

    /// world coordinates
    [[nodiscard]] glm::vec3 forward() const;
    
    /// world coordinates
    [[nodiscard]] glm::vec3 getPosition() const;

    /// forward: world coordinates
    void lookToward(const glm::vec3 &forward);

  private:
    static inline bool static_initialized_ = false;

    static inline float viewport_width;
    static inline float viewport_height;
    static inline double aspect_ratio_;

    static inline std::shared_ptr<resource::Texture> effect_;
    static inline Duration effect_duration_;
    static inline Instant effect_start_time_;
    bool display_effects_;

    static inline Instant shaking_start_;
    static inline Duration shaking_duration_;
    static inline glm::vec2 shaking_offset_;
    static inline Instant last_shake_;

    Data data_;
    /// local coordinates
    glm::vec3 forward_;

    std::weak_ptr<component::Transform> transform_;
    friend FreeViewControls;

    /// forward: local coordinates
    Camera3D(Data data, const glm::vec3 &forward, bool display_effects);

    
};

} // namespace component
