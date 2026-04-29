#pragma once

#include <filesystem>

[[nodiscard]] std::filesystem::path getExecutablePath();
[[nodiscard]] std::filesystem::path relativeToExeDir(const std::filesystem::path &path);
