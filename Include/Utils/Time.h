#pragma once

#include <chrono>

using Duration = std::chrono::nanoseconds;

using Instant = std::chrono::time_point<std::chrono::steady_clock>;

[[nodiscard]] Instant now();
[[nodiscard]] float toSeconds(Duration duration);
