#pragma once

#include <concepts>
#include <cstddef>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "Components/Component.h"

using GameObjectId = size_t;

class GameObject : public std::enable_shared_from_this<GameObject>
{
  public:
    bool visible = true;
    bool active = true;

    GameObject();

    void initialize();
    void update(float delta_time);
    void render() const;

    GameObjectId getId() const;

    [[nodiscard]] std::shared_ptr<GameObject> addChild();
    [[nodiscard]] std::optional<std::shared_ptr<GameObject>> getParent() const;
    [[nodiscard]] std::optional<std::shared_ptr<GameObject>> getGameObject(GameObjectId id);
    void detach();

    template <std::derived_from<component::Component> T, typename... Args>
    std::shared_ptr<T> addComponent(Args &&...args);

    template <std::derived_from<component::Component> T>
    [[nodiscard]] std::optional<std::shared_ptr<T>> getComponent() const;

    template <std::derived_from<component::Component> T> bool removeComponent();

    template <std::derived_from<component::Component> T>
    [[nodiscard]] std::optional<std::shared_ptr<T>> findFirstComponentInParents(bool search_self = true) const;

  private:
    inline static GameObjectId next_id_ = 0;
    const GameObjectId id_;

    std::vector<std::shared_ptr<component::Component>> components_;

    std::optional<std::weak_ptr<GameObject>> parent_;
    std::vector<std::shared_ptr<GameObject>> children_;

    bool initialized_ = false;
};

template <std::derived_from<component::Component> T, typename... Args>
std::shared_ptr<T> GameObject::addComponent(Args &&...args)
{
    auto component = std::make_shared<T>(std::forward<Args>(args)...);
    component->setOwner(shared_from_this());
    components_.push_back(component);
    return component;
}

template <std::derived_from<component::Component> T> std::optional<std::shared_ptr<T>> GameObject::getComponent() const
{
    for (auto &component : components_)
    {
        if (auto result = std::dynamic_pointer_cast<T>(component))
        {
            return std::optional(result);
        }
    }
    return std::nullopt;
}

template <std::derived_from<component::Component> T> bool GameObject::removeComponent()
{
    for (auto it = components_.begin(); it != components_.end(); ++it)
    {
        if (std::dynamic_pointer_cast<T>(it))
        {
            components_.erase(it);
            return true;
        }
    }
    return false;
}

template <std::derived_from<component::Component> T>
std::optional<std::shared_ptr<T>> GameObject::findFirstComponentInParents(bool search_self) const
{
    auto node_option = search_self ? std::optional(shared_from_this())
                                   : (parent_.has_value() ? std::optional(parent_->lock()) : std::nullopt);
    while (node_option.has_value())
    {
        const auto &node = node_option.value();

        const auto component_option = node->getComponent<T>();
        if (component_option.has_value())
        {
            return component_option;
        }
        else
        {
            node_option = (node->parent_.has_value() ? std::optional(node->parent_->lock()) : std::nullopt);
        }
    }

    return std::nullopt;
}
