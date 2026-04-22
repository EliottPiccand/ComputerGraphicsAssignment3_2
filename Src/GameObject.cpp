#include "GameObject.h"

#include <cassert>

#include <Lib/OpenGL.h>

GameObject::GameObject() : id_(next_id_)
{
    next_id_ += 1;
}

GameObjectId GameObject::getId() const
{
    return id_;
}

std::shared_ptr<GameObject> GameObject::addChild()
{
    auto child = std::make_shared<GameObject>();
    child->parent_ = std::optional(shared_from_this());
    children_.push_back(child);
    return child;
}

std::optional<std::shared_ptr<GameObject>> GameObject::getParent() const
{
    return parent_.has_value() ? std::optional(parent_->lock()) : std::nullopt;
}

std::optional<std::shared_ptr<GameObject>> GameObject::getGameObject(GameObjectId id)
{
    if (id == id_)
    {
        return std::optional(shared_from_this());
    }

    for (auto &child : children_)
    {
        auto obj = child->getGameObject(id);
        if (obj.has_value())
        {
            return obj;
        }
    }

    return std::nullopt;
}

void GameObject::detach()
{
    std::vector<std::shared_ptr<GameObject>> local_children;
    local_children.swap(children_);

    for (auto &child : local_children)
    {
        child->detach();
    }

    if (parent_.has_value())
    {
        auto &siblings = parent_->lock()->children_;
        std::erase_if(siblings, [&](auto s) { return s->id_ == id_; });

        parent_ = std::nullopt;
    }
}

void GameObject::initialize()
{
    if (initialized_)
    {
        return;
    }

    for (auto &component : components_)
    {
        component->initialize();
    }

    for (auto &child : children_)
    {
        child->initialize();
    }

    initialized_ = true;
}

void GameObject::update(float delta_time)
{
    if (!active)
    {
        return;
    }

    assert(initialized_ && "GameObject::update called while uninitialized");

    for (auto &child : children_)
    {
        child->update(delta_time);
    }

    for (auto &component : components_)
    {
        component->update(delta_time);
    }
}

void GameObject::render() const
{
    if (!visible)
    {
        return;
    }

    assert(initialized_ && "GameObject::render called while uninitialized");

    size_t matrices_to_pop = 0;

    for (const auto &component : components_)
    {
        if (component->render())
        {
            matrices_to_pop += 1;
        }
    }

    for (const auto &child : children_)
    {
        child->render();
    }

    while (matrices_to_pop > 0)
    {
        glPopMatrix();
        matrices_to_pop -= 1;
    }
}
