#pragma once

#include <Lib/glm.h>
#include <cstdlib>
#include <cstring>
#include <tuple>
#include <utility>

#include "Events/Event.h"

namespace event
{

struct SpawnParticles : public Event
{
    enum class Type
    {
        Explosion,
        Smoke,
        FoamTrail,
        WaterSplash,
        CannonBallSpark,
    };

    const Type type;
    const glm::vec3 position;
    const size_t count;
    const void *additional_data;

    SpawnParticles(const Type type, const glm::vec3 &position, const size_t count,
                   const void *additional_data = nullptr);

    template <typename... T>
    [[nodiscard]] static void *createAdditionalData(T... data);

    template <typename... T>
    static const std::tuple<T...> getAdditionalData(const void *data);
};

template <typename... T>
void *SpawnParticles::createAdditionalData(T... data)
{
    const std::tuple<T...> data_as_tuple = std::make_tuple(std::forward<T>(data)...);
    void *ptr = std::malloc(sizeof(data_as_tuple));
    *static_cast<std::tuple<T...>*>(ptr) = data_as_tuple;
    return ptr;
}

template <typename... T>
const std::tuple<T...> SpawnParticles::getAdditionalData(const void *data)
{
    return *static_cast<const std::tuple<T...>*>(data);
}

} // namespace event
