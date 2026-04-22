#include "Components/Animation.h"

#include "GameObject.h" // IWYU pragma: keep

using namespace component;

Animation::Animation(Callback callback) : callback_(callback)
{
}

void Animation::initialize()
{
    GET_COMPONENT(Transform, transform_, Animation);
}

void Animation::update(float delta_time)
{
    auto transform = transform_.lock();
    auto game_object = owner_.lock();
    callback_(delta_time, transform, game_object);
}
