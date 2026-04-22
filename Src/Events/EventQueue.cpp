#include "Events/EventQueue.h"

void EventQueue::processAll()
{
    for (size_t i = 0; i < events_.size(); ++i)
    {
        const auto &raw_event = events_[i];
        if (raw_event == nullptr)
            continue;

        const event::Event &event = *raw_event;
        const std::type_index event_type_id = typeid(event);

        const auto it = callbacks_.find(event_type_id);
        if (it == callbacks_.end())
            continue;

        for (const auto &wrapper : it->second)
        {
            wrapper(event);
        }
    }

    events_.clear();
}
