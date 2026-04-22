#pragma once

#include <Lib/glm.h>

#include "Events/Event.h"
#include "GameObject.h"

namespace event
{

struct Fire : public Event
{
    const glm::vec3 position;
    const glm::vec3 initial_velocity;
    const GameObjectId shooter;

    Fire(const glm::vec3 &position, const glm::vec3 &initial_velocity, const GameObjectId shooter);
};

} // namespace event
