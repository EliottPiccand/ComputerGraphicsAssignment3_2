#include "Events/Fire.h"

using namespace event;

Fire::Fire(const glm::vec3 &pos, const glm::vec3 &vel, const GameObjectId shooter_id)
    : position(pos), initial_velocity(vel), shooter(shooter_id)
{
}
