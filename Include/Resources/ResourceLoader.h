#pragma once

#include <concepts>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include "Utils/Log.h"
#include "Utils/Path.h"

template <typename T>
concept Resource = requires {
    { T::DIRECTORY } -> std::same_as<const std::string_view &>;
};

class ResourceLoader
{
  public:
    static inline const std::filesystem::path ASSETS_DIRECTORY = [] { return getExecutablePath() / "Assets"; }();

    template <Resource R, typename... Args> static void load(std::string_view name, Args &&...args);
    template <Resource R> [[nodiscard]] static std::shared_ptr<R> get(std::string_view name);
    [[nodiscard]] static bool isLoaded(std::string_view name);

  private:
    static inline std::unordered_map<std::string, std::shared_ptr<void>> resources_;
};

template <Resource R, typename... Args> void ResourceLoader::load(std::string_view name, Args &&...args)
{
    const auto full_name = std::format("{}/{}", R::DIRECTORY, name);

    if (resources_.contains(full_name))
    {
        LOG_WARNING("resource '{}' already loaded, skipping", full_name);
        return;
    }

    resources_[full_name] = R::load(std::forward<Args>(args)...);
}

template <Resource R> std::shared_ptr<R> ResourceLoader::get(std::string_view name)
{
    const auto full_name = std::format("{}/{}", R::DIRECTORY, name);

    if (!resources_.contains(full_name))
    {
        LOG_ERROR("resource '{}' not loaded", full_name);
        throw std::runtime_error("resource not loaded");
    }

    return std::static_pointer_cast<R>(resources_[full_name]);
}
