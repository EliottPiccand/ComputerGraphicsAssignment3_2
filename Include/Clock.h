#pragma once

#include "Utils/Time.h"
#include <cstddef>

class Clock
{
  public:
    Clock();

    [[nodiscard]] float tick();
    [[nodiscard]] float getFps();

  private:
    Instant last_frame_;

    Duration frame_time_sum_;
    size_t frame_count_ = 0;
};
