#pragma once

#include <concepts>
#include <ranges>

template <typename R>
concept Range = std::ranges::input_range<R>;

template <typename R, typename T>
concept RangeOf = std::ranges::input_range<R> && std::same_as<std::ranges::range_value_t<R>, T>;
