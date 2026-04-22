#pragma once

#include <cstdint>

#include "Events/Event.h"

namespace event
{

struct WindowResized : public Event
{
    uint32_t width;
    uint32_t height;

    WindowResized(uint32_t width, uint32_t height);
};

} // namespace event
