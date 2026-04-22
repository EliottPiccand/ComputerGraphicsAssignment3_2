#pragma once

#include "Events/Event.h"
#include "GameObject.h"

namespace event
{

struct DetachGameObject : public Event
{
    const GameObjectId game_object_id;

    DetachGameObject(const GameObjectId game_object_id);
};

} // namespace event
