#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "Events/Event.h"

class EventQueue
{
  public:
    template <std::derived_from<event::Event> T> using Callback = std::function<void(const T &)>;

    template <std::derived_from<event::Event> T> static void registerCallback(Callback<T> callback);
    template <std::derived_from<event::Event> EventType, typename... Args> static void post(Args... args);
    static void processAll();

  private:
    using GenericCallback = std::function<void(const event::Event &)>;

    static inline std::vector<std::unique_ptr<event::Event>> events_;
    static inline std::unordered_map<std::type_index, std::vector<GenericCallback>> callbacks_;
};

template <std::derived_from<event::Event> EventType, typename... Args> void EventQueue::post(Args... args)
{
    events_.push_back(std::make_unique<EventType>(args...));
}

template <std::derived_from<event::Event> T> void EventQueue::registerCallback(Callback<T> callback)
{
    const std::type_index type_index = typeid(T);

    auto wrapper_function = [callback](const event::Event &base_event) {
        const T *typed_event = dynamic_cast<const T *>(&base_event);
        if (typed_event)
        {
            callback(*typed_event);
        }
    };

    callbacks_[type_index].push_back({std::move(wrapper_function)});
}
