#pragma once

#include <cstddef>

#include "Utils/Time.h"

class Clock
{
  public:
    Clock();

    float tick();
    [[nodiscard]] float getFps();

  private:
    Instant last_frame_;

    Duration frame_time_sum_;
    size_t frame_count_ = 0;
};
