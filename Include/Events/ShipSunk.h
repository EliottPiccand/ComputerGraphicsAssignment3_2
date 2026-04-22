#pragma once

#include "Events/Event.h"
#include "GameObject.h"

namespace event
{

struct ShipSunk : public Event
{
    const GameObjectId game_object_id;

    ShipSunk(const GameObjectId game_object_id);
};

} // namespace event
