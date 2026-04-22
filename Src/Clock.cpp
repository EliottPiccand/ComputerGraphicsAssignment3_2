#include "Clock.h"

#include <chrono>

Clock::Clock() : last_frame_(now()), frame_time_sum_(0)
{
}

float Clock::tick()
{
    const Duration elapsed = now() - last_frame_;

    frame_count_ += 1;
    frame_time_sum_ += elapsed;

    last_frame_ = now();

    return std::chrono::duration<float>(elapsed).count();
}

float Clock::getFps()
{
    const float fps = static_cast<float>(frame_count_) / std::chrono::duration<float>(frame_time_sum_).count();
    frame_count_ = 0;
    frame_time_sum_ = Duration(0);
    return fps;
}
