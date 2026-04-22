#pragma once

#include <functional>
#include <memory>

#include "Components/Component.h"
#include "GameObject.h"

namespace component
{

class Health : public Component
{
  public:
    using OnDeathCallback = std::function<void(std::shared_ptr<GameObject> game_object)>;

    Health(float max_hit_points, OnDeathCallback on_death_callback);

    void initialize() override;

    void damage(float hit_points);
    void heal(float hit_points);
    void heal();
    
    float getRemainingHealthRatio() const;

  private:
    const float max_hit_points_;
    float hit_points_;

    OnDeathCallback on_death_callback_;
};

} // namespace component
