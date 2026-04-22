#include "Components/Component.h"

#include "GameObject.h"

using namespace component;

void Component::initialize()
{
}

void Component::update(float delta_time)
{
    (void)delta_time;
}

bool Component::render() const
{
    return false;
}

void Component::setOwner(std::shared_ptr<GameObject> game_object)
{
    owner_ = game_object;
}

std::shared_ptr<GameObject> Component::getOwner() const
{
    return owner_.lock();
}
