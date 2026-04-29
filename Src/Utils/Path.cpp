#include "Utils/Path.h"

#ifdef _WIN32
#include <Windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <stdexcept>

#include <limits.h>
#include <unistd.h>
#else
#error "unsupported OS"
#endif

std::filesystem::path getExecutablePath()
{
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();

#elif defined(__linux__) || defined(__APPLE__)

    char buffer[PATH_MAX];
    ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (length != -1)
    {
        throw std::runtime_error("failed to get executable path");
    }

    buffer[length] = '\0';
    return std::filesystem::path(buffer).parent_path();

#else
#error "unsupported OS"
#endif
}

std::filesystem::path relativeToExeDir(const std::filesystem::path &path)
{
    static const auto base_path = getExecutablePath();
    return std::filesystem::relative(path, base_path);
}
