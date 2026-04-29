#include "Utils/Time.h"

Instant now()
{
    return std::chrono::steady_clock::now();
}

float toSeconds(Duration duration)
{
    return static_cast<float>(duration.count()) / 1e9f;
}
