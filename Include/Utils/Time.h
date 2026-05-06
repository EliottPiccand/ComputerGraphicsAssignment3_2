#pragma once

class Duration;
class Instant;
class Time;

class Duration
{
  public:
    constexpr Duration() = default;
    constexpr Duration(float seconds) : seconds_(seconds)
    {
    }

    static inline constexpr Duration seconds(float seconds)
    {
        return Duration(seconds);
    }
    static inline constexpr Duration milliseconds(float milliseconds)
    {
        return Duration(milliseconds * 0.001f);
    }

    constexpr float toSeconds() const
    {
        return seconds_;
    }

    constexpr bool operator>(const Duration &other) const
    {
        return seconds_ > other.seconds_;
    }

    constexpr bool operator>=(const Duration &other) const
    {
        return seconds_ >= other.seconds_;
    }

    constexpr bool operator<(const Duration &other) const
    {
        return seconds_ < other.seconds_;
    }

    constexpr bool operator<=(const Duration &other) const
    {
        return seconds_ <= other.seconds_;
    }

    constexpr Duration operator*(float scale) const
    {
        return Duration(seconds_ * scale);
    }

    constexpr Duration operator+(const Duration &other) const
    {
        return Duration(seconds_ + other.seconds_);
    }

  private:
    friend Time;
    friend Instant;
    float seconds_ = 0.0f;
};

class Instant
{
  public:
    constexpr Instant() = default;
    constexpr Instant(float seconds) : seconds_(seconds)
    {
    }

    constexpr Duration operator-(const Instant &other) const
    {
        return Duration(seconds_ - other.seconds_);
    }
    constexpr Instant operator+(const Duration &other) const
    {
        return Instant(seconds_ + other.seconds_);
    }
    constexpr Instant operator-(const Duration &other) const
    {
        return Instant(seconds_ - other.seconds_);
    }

    constexpr bool operator>(const Instant &other) const
    {
        return seconds_ > other.seconds_;
    }

    constexpr bool operator>=(const Instant &other) const
    {
        return seconds_ >= other.seconds_;
    }

    constexpr bool operator<(const Instant &other) const
    {
        return seconds_ < other.seconds_;
    }

    constexpr bool operator<=(const Instant &other) const
    {
        return seconds_ <= other.seconds_;
    }

  private:
    friend Time;

    float seconds_ = 0.0f;
};

class Time
{
  public:
    static inline bool paused;

    [[nodiscard]] static Instant now();
    [[nodiscard]] static Duration elapsed();

    static void initialize();
    static void update(float delta_time);

    static float getDeltaTime();
    static float getDeltaTimeNoPause();

  private:
    static inline float delta_time_;
    static inline Instant now_;
};
