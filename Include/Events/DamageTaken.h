#pragma once

#include "Events/Event.h"
#include "GameObject.h"

namespace event
{

struct DamageTaken : public Event
{
    const GameObjectId game_object_id;

    DamageTaken(const GameObjectId game_object_id);
};

} // namespace event
