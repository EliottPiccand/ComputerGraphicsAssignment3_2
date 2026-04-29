#pragma once

#include <concepts>
#include <filesystem>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>

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

template <typename R, typename Factory, typename... FactoryArgs>
concept ResourceFactory = std::invocable<Factory, FactoryArgs...> && requires(Factory &&factory,
                                                                              FactoryArgs &&...factory_args) {
    {
        std::apply([]<typename... Ts>(Ts &&...xs) -> std::shared_ptr<R> { return R::load(std::forward<Ts>(xs)...); },
                   std::invoke(std::forward<Factory>(factory), std::forward<FactoryArgs>(factory_args)...))
    } -> std::same_as<std::shared_ptr<R>>;
};

class ResourceLoader
{
  public:
    template <Asset A> static std::shared_ptr<A> getAsset(const std::filesystem::path &path);

    template <typename R, typename... Args>
        requires Resource<R, Args...>
    static void load(const std::string &name, Args &&...args);
    template <typename R> static std::shared_ptr<R> get(const std::string &name);
    template <typename R, typename Factory, typename... FactoryArgs>
    static std::shared_ptr<R> getOrFactoryLoad(const std::string &name, Factory &&factory,
                                               FactoryArgs &&...factory_args);

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

        resources_[name] = A::loadFromFile(full_path);
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

    resources_[name] = R::load(std::forward<Args>(args)...);
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

template <typename R, typename Factory, typename... FactoryArgs>
std::shared_ptr<R> ResourceLoader::getOrFactoryLoad(const std::string &name, Factory &&factory,
                                                    FactoryArgs &&...factory_args)
{
    static_assert(ResourceFactory<R, Factory, FactoryArgs...>,
                  "Factory must be invocable with FactoryArgs... and return a tuple "
                  "that can be applied to R::load to produce std::shared_ptr<R>");

    if (!resources_.contains(name))
    {
        if (Singleton::game_loaded)
        {
            LOG_WARNING("resource '{}' should be loaded during program startup", name);
        }

        resources_[name] = std::apply(
            []<typename... Ts>(Ts &&...xs) -> std::shared_ptr<R> { return R::load(std::forward<Ts>(xs)...); },
            std::invoke(std::forward<Factory>(factory), std::forward<FactoryArgs>(factory_args)...));
    }

    return std::static_pointer_cast<R>(resources_[name]);
}
