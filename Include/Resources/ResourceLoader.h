#pragma once

#include <concepts>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <unordered_map>

#include "Singleton.h"
#include "Utils/Log.h"
#include "Utils/Path.h"

template <typename T>
concept Asset = requires {
    { T::DIRECTORY } -> std::same_as<const std::string_view &>;
    { T::loadFromFile(std::declval<const std::filesystem::path &>()) } -> std::same_as<std::shared_ptr<T>>;
};

template <typename T, typename... Args>
concept Resource = requires {
    { T::load(std::declval<Args>()...) } -> std::same_as<std::shared_ptr<T>>;
};

class ResourceLoader
{
  public:
    template <Asset A> static std::shared_ptr<A> getAsset(const std::filesystem::path &path);

    template <typename R, typename... Args>
        requires Resource<R, Args...>
    static void load(const std::string &name, Args &&...args);
    template <typename R> static std::shared_ptr<R> get(const std::string &name);

  private:
    static inline std::unordered_map<std::string, std::shared_ptr<void>> resources_;
    static inline const std::filesystem::path ASSET_PATH = [] { return getExecutablePath() / "Assets"; }();
};

template <Asset A> std::shared_ptr<A> ResourceLoader::getAsset(const std::filesystem::path &path)
{
    const std::filesystem::path full_path = ASSET_PATH / A::DIRECTORY / path;
    const std::string name = path.string();

    const auto it = resources_.find(name);
    if (it == resources_.end())
    {
        if (Singleton::game_loaded)
        {
            LOG_WARNING("asset '{}' should be loaded during program startup", name);
        }

        LOG_DEBUG("loading asset {}", name);
        resources_[name] = A::loadFromFile(full_path);
        LOG_DEBUG("loaded asset {}", name);
    }

    return std::static_pointer_cast<A>(resources_[name]);
}

template <typename R, typename... Args>
    requires Resource<R, Args...>
void ResourceLoader::load(const std::string &name, Args &&...args)
{
    if (Singleton::game_loaded)
    {
        LOG_WARNING("resource '{}' should be loaded during program startup", name);
    }

    const auto it = resources_.find(name);
    if (it != resources_.end())
    {
        LOG_WARNING("resource '{}' has already been loaded, skipping", name);
        return;
    }

    LOG_DEBUG("loading resource '{}'", name);
    resources_[name] = R::load(std::forward<Args>(args)...);
    LOG_DEBUG("loaded resource '{}'", name);
}

template <typename R> std::shared_ptr<R> ResourceLoader::get(const std::string &name)
{
    const auto it = resources_.find(name);
    if (it == resources_.end())
    {
        LOG_ERROR("resource '{}' has never been loaded", name);
        throw std::runtime_error("accessing unloaded resource");
    }

    return std::static_pointer_cast<R>(resources_[name]);
}
