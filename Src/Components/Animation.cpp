#include "Components/Animation.h"

#include "Utils/Profiling.h"

using namespace component;

Animation::Animation(Callback callback) : callback_(callback)
{
}

void Animation::initialize()
{
    ProfileScope;

    GET_COMPONENT(Transform, transform_, Animation);
}

void Animation::update()
{
    ProfileScope;

    auto transform = transform_.lock();
    auto game_object = owner_.lock();
    callback_(transform, game_object);
}
