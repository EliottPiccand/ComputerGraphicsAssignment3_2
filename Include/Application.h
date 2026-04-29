#pragma once

#include <memory>
#include <optional>
#include <unordered_map>

#include "Clock.h"
#include "Components/Camera3D.h"
#include "Components/FreeViewControls.h"
#include "GameObject.h"
#include "Utils/View.h"
#include "Window.h"

class Application
{
  public:
    Application();
    void run();

  private:
    Clock clock_;
    bool should_close_;

    std::unique_ptr<Window> window_;

    std::shared_ptr<GameObject> scene_root_;
    std::vector<std::pair<std::weak_ptr<GameObject>, std::weak_ptr<GameObject>>> ships_and_health_bars_;
    std::unordered_map<GameObjectId, std::weak_ptr<GameObject>> to_detach_on_restart_;

    std::weak_ptr<component::Camera3D> free_view_camera_;
    std::weak_ptr<component::FreeViewControls> free_view_controls_;

    bool free_view_override_;
    View main_view_;
    std::weak_ptr<component::Camera3D> top_view_camera_;
    std::weak_ptr<component::Camera3D> cannon_camera_;
    std::optional<std::weak_ptr<component::Camera3D>> last_cannon_ball_camera_;

    std::weak_ptr<GameObject> victory_message_;
    std::weak_ptr<GameObject> defeat_message_;

    GameObjectId player_id_;

    void initializeOpenGL();

    void update(float delta_time);
    void render() const;
    void renderPass(std::shared_ptr<component::Camera3D> camera) const;

    void restart();

    void updateActiveView();
};
