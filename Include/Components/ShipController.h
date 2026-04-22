#pragma once

#include <memory>

#include "Components/Component.h"
#include "Components/RigidBody.h"
#include "Components/Transform.h"

#include <Lib/glm.h>

namespace component
{

/// This class should never
class ShipController : public Component
{
  public:
    static inline constexpr const float SHIP_SPEED = 6.0f;                    // m/s
    static inline constexpr const float SHIP_TURN_STEP = glm::radians(15.0f); // rad

    enum class SpeedState
    {
        Forward,
        Stopped,
        Backward,
    };

    enum class TurnState
    {
        Left,
        None,
        Right,
    };

    ShipController();
    virtual ~ShipController() override = default;

    void initialize() override;
    void update(float delta_time) override;

  protected:
    virtual void updateStates();

    SpeedState speed_state_;
    TurnState turn_state_;
    float turn_speed_;

    std::weak_ptr<Transform> transform_;
    std::weak_ptr<RigidBody> rigid_body_;
};

} // namespace component
