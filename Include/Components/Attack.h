#pragma once

#include <memory>
#include <unordered_map>

#include "Components/Component.h"
#include "Components/Health.h"
#include "GameObject.h"
#include "Utils/Time.h"

namespace component
{

class Attack : public Component
{
  public:
    Attack(float min_damages, float max_damages, Duration min_hit_delay);

    void update(float delta_time) override;
    void dealDamages(std::shared_ptr<Health> to);

  private:
    const float min_damages_;
    const float max_damages_;
    const Duration min_hit_delay_;

    std::unordered_map<GameObjectId, Instant> last_hits_;
};

} // namespace component
