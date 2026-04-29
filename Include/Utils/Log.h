#pragma once

#include <print> // IWYU pragma: keep

#define LOG_COLOR_RESET  "\033[0m"
#define LOG_COLOR_WHITE  "\033[37m"
#define LOG_COLOR_GREEN  "\033[32m"
#define LOG_COLOR_BLUE   "\033[34m"
#define LOG_COLOR_YELLOW "\033[1;33m"
#define LOG_COLOR_RED    "\033[1;31m"

#if defined(OE_DEBUG) || defined(OE_PROFILING)

#define LOG_TRACE(message, ...)   std::println(LOG_COLOR_WHITE  "[Trace] "   message LOG_COLOR_RESET __VA_OPT__(,) __VA_ARGS__)
#define LOG_DEBUG(message, ...)   std::println(LOG_COLOR_GREEN  "[Debug] "   message LOG_COLOR_RESET __VA_OPT__(,) __VA_ARGS__)
#define LOG_INFO(message, ...)    std::println(LOG_COLOR_BLUE   "[Info] "    message LOG_COLOR_RESET __VA_OPT__(,) __VA_ARGS__)
#define LOG_WARNING(message, ...) std::println(LOG_COLOR_YELLOW "[Warning] " message LOG_COLOR_RESET __VA_OPT__(,) __VA_ARGS__)
#define LOG_ERROR(message, ...)   std::println(LOG_COLOR_RED    "[Error] "   message LOG_COLOR_RESET __VA_OPT__(,) __VA_ARGS__)

#elif defined(OE_RELEASE)

#define LOG_TRACE(message, ...)
#define LOG_DEBUG(message, ...)
#define LOG_INFO(message, ...)    std::println(LOG_COLOR_BLUE   "[Info] "    message LOG_COLOR_RESET __VA_OPT__(,) __VA_ARGS__)
#define LOG_WARNING(message, ...) std::println(LOG_COLOR_YELLOW "[Warning] " message LOG_COLOR_RESET __VA_OPT__(,) __VA_ARGS__)
#define LOG_ERROR(message, ...)   std::println(LOG_COLOR_RED    "[Error] "   message LOG_COLOR_RESET __VA_OPT__(,) __VA_ARGS__)

#else 
#error "Inproper configuration"
#endif
