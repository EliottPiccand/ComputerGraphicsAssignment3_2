#pragma once

#include <memory>

#include "Components/Camera3D.h"
#include "GameObject.h"
#include "Utils/RenderingStyle.h"
#include "Utils/View.h"

struct Singleton
{
    static inline bool game_loaded = false;
    static inline bool debug = false;

    static inline View view;
    static inline std::weak_ptr<component::Camera3D> active_camera;

    static inline RenderingStyle rendering_style = RenderingStyle::OpaquePolygon;

    static inline std::weak_ptr<GameObject> scene_root;
    static inline bool physics_paused = false;
};
