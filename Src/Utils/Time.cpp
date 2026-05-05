#include "Utils/Time.h"

Instant Time::now()
{
    return now_;
}

void Time::initialize()
{
    paused = false;
    now_ = Instant();
    delta_time_ = 0.0f;
}

void Time::update(float delta_time)
{
    delta_time_ = delta_time;

    if (paused)
        return;

    now_.seconds_ += delta_time;
}

float Time::getDeltaTime()
{
    return paused ? 0.0f : delta_time_;
}

float Time::getDeltaTimeNoPause()
{
    return delta_time_;
}
