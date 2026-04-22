#pragma once

#include <memory>

class GameObject;

namespace component
{

class Component : public std::enable_shared_from_this<Component>
{
  public:
    virtual ~Component() = default;

    virtual void initialize();
    virtual void update(float delta_time);
    virtual bool render() const; /// return true if a model-view matrix has been pushed onto OpenGL stack

    void setOwner(std::shared_ptr<GameObject> game_object);
    [[nodiscard]] std::shared_ptr<GameObject> getOwner() const;

  protected:
    std::weak_ptr<GameObject> owner_;
};

} // namespace component

#define GET_COMPONENT(ComponentType, variable, ClassType)                                                              \
    const auto variable##Option = owner_.lock()->findFirstComponentInParents<ComponentType>();                         \
    assert(variable##Option.has_value() && "No transform found! component::" #ClassType                                \
                                           " needs its node or one of its parents has a component::" #ComponentType);  \
    variable = variable##Option.value()
