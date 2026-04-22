#include "Utils/Time.h"

Instant now()
{
    return std::chrono::steady_clock::now();
}
