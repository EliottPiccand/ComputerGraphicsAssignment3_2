#include "Clock.h"

#include "Utils/Time.h"

namespace
{

std::chrono::time_point<std::chrono::steady_clock> now()
{
    return std::chrono::steady_clock::now();
}

} // namespace

Clock::Clock() : last_frame_(now())
{
}

Duration Clock::tick()
{
    const auto elapsed = now() - last_frame_;
    last_frame_ = now();

    return Duration::seconds(std::chrono::duration<float>(elapsed).count());
}
