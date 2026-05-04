#pragma once

#include <chrono>

#include "Utils/Time.h"

class Clock
{
  public:
    Clock();

    [[nodiscard]] Duration tick();

  private:
    std::chrono::time_point<std::chrono::steady_clock> last_frame_;
};
