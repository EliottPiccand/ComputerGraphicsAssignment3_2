#pragma once

#include <cstddef>
#include <random>
#include <vector>

#include <Lib/glm.h>

#include "Utils/Ranges.h"

class Random
{
  public:
    static void initialize();
    [[nodiscard]] static float random(float min, float max);
    [[nodiscard]] static int randint(int min, int max);
    [[nodiscard]] static float radians();
    /// along must be normalized, spread is in radians
    [[nodiscard]] static glm::vec3 direction(const glm::vec3 &along, float spread);
    [[nodiscard]] static glm::vec3 direction();

    template <Range R> [[nodiscard]] static size_t index(const R &range);

    template <typename T, RangeOf<T> R> [[nodiscard]] static const T &range(const R &range);
    template <typename T> [[nodiscard]] static T pop(std::vector<T> &range);

  private:
    static inline std::random_device random_device_;
    static inline std::mt19937 generator_;
};

template <Range R> size_t Random::index(const R &range)
{
    std::uniform_int_distribution<size_t> distribution(0, range.size() - 1);
    return distribution(generator_);
}

template <typename T, RangeOf<T> R> const T &Random::range(const R &range)
{
    return range[Random::index(range)];
}

template <typename T> T Random::pop(std::vector<T> &range)
{
    const auto index = Random::index(range);
    T value = range[index];

    range.erase(range.begin() + static_cast<typename std::vector<T>::difference_type>(index));

    return value;
}
