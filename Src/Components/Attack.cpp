#include "Components/Attack.h"

#include "Utils/Random.h"

using namespace component;

Attack::Attack(float min_damages, float max_damages, Duration min_hit_delay)
    : min_damages_(min_damages), max_damages_(max_damages), min_hit_delay_(min_hit_delay)
{
}

void Attack::update(float delta_time)
{
    (void)delta_time;

    std::erase_if(last_hits_, [this](auto &key_value_pair){
        auto &[key, value] = key_value_pair;
        return now() >= value + min_hit_delay_;
    });
}

void Attack::dealDamages(std::shared_ptr<Health> to)
{
    const auto game_object_id = to->getOwner()->getId();

    if (last_hits_.contains(game_object_id) && now() - last_hits_[game_object_id] < min_hit_delay_)
        return;
    
    last_hits_[game_object_id] = now();

    float damages = Random::random(min_damages_, max_damages_);
    to->damage(damages);
}
