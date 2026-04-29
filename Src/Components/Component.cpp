#include "Components/Component.h"

using namespace component;

void Component::initialize()
{
}

void Component::update(float delta_time)
{
    (void)delta_time;
}

void Component::render(glm::mat4 &transform) const
{
    (void)transform;
}

void Component::setOwner(std::shared_ptr<GameObject> game_object)
{
    owner_ = game_object;
}

std::shared_ptr<GameObject> Component::getOwner() const
{
    return owner_.lock();
}
