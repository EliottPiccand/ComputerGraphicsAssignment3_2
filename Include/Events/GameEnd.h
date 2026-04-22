#pragma once

#include "Events/Event.h"

namespace event
{

struct GameEnd : public Event
{
    const bool victory;

    GameEnd(bool victory);
};

} // namespace event
