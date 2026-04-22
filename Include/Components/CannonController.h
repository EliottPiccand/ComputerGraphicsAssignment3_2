#pragma once

#include <memory>

#include <Lib/glm.h>

#include "Components/Component.h"
#include "Components/Transform.h"
#include "GameObject.h"

namespace component
{

class CannonController : public Component
{
  public:
    CannonController(std::weak_ptr<Transform> cannon_barrel_transform, std::weak_ptr<Transform> target_transform);
    virtual ~CannonController() override = default;

    void initialize() override;
    void update(float delta_time) override;
    bool render() const override;

    virtual void updateTarget(float delta_time);

  protected:
    glm::vec3 getShootingInitialVelocity(const glm::vec3 &target) const;

    std::weak_ptr<Transform> transform_;
    std::weak_ptr<Transform> barrel_transform_;
    std::weak_ptr<Transform> target_transform_;

    bool fired_;
    bool aiming_;
    float recoil_;

    glm::vec3 target_;
    glm::vec3 cannon_ball_initial_velocity_;

  private:
    GameObjectId shooter_id_;
};

} // namespace component
