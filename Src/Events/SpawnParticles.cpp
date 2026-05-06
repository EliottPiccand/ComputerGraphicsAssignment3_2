#include "Events/SpawnParticles.h"

using namespace event;

SpawnParticles::SpawnParticles(Type type_, const glm::vec3 &position_, const size_t count_,
                               const void *additional_data_)
    : type(type_), position(position_), count(count_), additional_data(additional_data_)
{
}
