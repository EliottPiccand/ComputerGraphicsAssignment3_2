#include "Application.h"

#include <array>
#include <numbers>
#include <string_view>

#include <Lib/OpenGL.h>
#include <Lib/glm.h>

#include "Components/Animation.h"
#include "Components/Attack.h"
#include "Components/CannonAIController.h"
#include "Components/CannonPlayerController.h"
#include "Components/Collider.h"
#include "Components/DirectionalLight.h"
#include "Components/Flag.h"
#include "Components/HealthBar.h"
#include "Components/ModelInstance.h"
#include "Components/RigidBody.h"
#include "Components/ShipAIController.h"
#include "Components/ShipPlayerController.h"
#include "Components/Sky.h"
#include "Components/Text.h"
#include "Components/Water.h"
#include "Events/DamageTaken.h"
#include "Events/DetachGameObject.h"
#include "Events/EventQueue.h"
#include "Events/Fire.h"
#include "Events/GameEnd.h"
#include "Events/ShipSunk.h"
#include "Events/SpawnParticles.h"
#include "Events/WindowResized.h"
#include "Input.h"
#include "Mesh/Vertex/VertexParticle.h"
#include "ParticleSystem.h"
#include "Physics.h"
#include "Resources/ComputeShader.h"
#include "Resources/Model.h"
#include "Resources/ResourceLoader.h"
#include "Resources/Shader.h"
#include "Resources/Texture.h"
#include "Singleton.h"
#include "Utils/Color.h"
#include "Utils/Constants.h"
#include "Utils/Log.h"
#include "Utils/Math.h"
#include "Utils/Profiling.h"
#include "Utils/Random.h"
#include "Utils/Time.h"

constexpr const bool DEBUG_SCENE = false;

#pragma region model_settings

constexpr const glm::vec3 SHIP_MODEL_TRANSLATION = -0.5f * MODEL_RIGHT;
constexpr const glm::vec3 SHIP_MODEL_ROTATION = {glm::radians(90.0f), 0.0f, glm::radians(180.0f)};
constexpr const glm::vec3 SHIP_MODEL_SCALE = 0.5f * ONE;
constexpr const float SHIP_MASS = 10'000.0f; // kg
const component::Collider::ConvexPolyhedron SHIP_MODEL_COLLIDER = {
    .vertices =
        {
            {-2.5f, -10.0f, 5.5f},
            {2.5f, -10.0f, 5.5f},
            {-2.5f, -10.0f, 0.0f},
            {2.5f, -10.0f, 0.0f},
            {-2.5f, 8.0f, 4.0f},
            {2.5f, 8.0f, 4.0f},
            {-2.5f, 8.0f, 0.0f},
            {2.5f, 8.0f, 0.0f},
            {0.0f, 12.0f, 4.0f},
            {0.0f, 11.5f, 0.5f},
        },
    .faces =
        {
            {0, 1, 3},
            {0, 3, 2},
            {0, 2, 6},
            {0, 6, 4},
            {1, 5, 7},
            {1, 7, 3},
            {0, 4, 5},
            {0, 5, 1},
            {2, 3, 7},
            {2, 7, 6},
            {4, 6, 9},
            {4, 9, 8},
            {5, 8, 9},
            {5, 9, 7},
            {6, 7, 9},
            {4, 8, 5},
        },
};

constexpr const glm::vec3 SHIP_FLAG_TRANSLATION = MODEL_UP * 17.8f + MODEL_FORWARD * 1.85f;
constexpr const glm::vec3 SHIP_FLAG_ROTATION = {glm::radians(90.0f), glm::radians(180.0f), glm::radians(90.0f)};
constexpr const std::string_view ENEMY_SHIP_FLAG = "Ship/SailsRopeAlbedo.png";

constexpr const glm::vec3 CANNON_STAND_MODEL_TRANSLATION = {0.0f, 0.0f, 1.0f};
constexpr const glm::vec3 CANNON_STAND_MODEL_ROTATION = {glm::radians(90.0f), 0.0f, glm::radians(180.0f)};
constexpr const glm::vec3 CANNON_STAND_MODEL_SCALE = ONE;

constexpr const glm::vec3 CANNON_BARREL_MODEL_TRANSLATION = ZERO;
constexpr const glm::vec3 CANNON_BARREL_MODEL_ROTATION = {glm::radians(99.0f), 0.0f, glm::radians(180.0f)};
constexpr const glm::vec3 CANNON_BARREL_MODEL_SCALE = ONE;

constexpr const glm::vec3 CANNON_POSITION_IN_SHIP = 9.0f * MODEL_FORWARD + 3.7f * MODEL_UP;
constexpr const glm::vec3 CANNON_BARREL_POSITION_IN_CANNON = 1.0f * MODEL_UP;
constexpr const glm::vec3 CANNON_BARREL_ROTATION_IN_CANNON = {glm::radians(-90.0f), glm::radians(90.0f), 0.0f};

constexpr const glm::vec3 CANNON_BALL_MODEL_TRANSLATION = ZERO;
constexpr const glm::vec3 CANNON_BALL_MODEL_ROTATION = {0.0f, 0.0f, glm::radians(180.0f)};
constexpr const glm::vec3 CANNON_BALL_MODEL_SCALE = 0.4f * ONE;
constexpr const float CANNON_BALL_TOP_VIEW_SCALE_FACTOR = 5.0f;
constexpr const float CANNON_BALL_MASS = 10.0f;
constexpr const component::Collider::AABB CANNON_BALL_COLLIDER = {
    .half_size = 0.2f * ONE,
    .center = ZERO,
};

constexpr const float RADAR_CYLINDER_HEIGHT = 1.0f;
constexpr const float RADAR_CYLINDER_RAIDUS = 0.2f;
constexpr const size_t RADAR_CYLINDER_RESOLUTION = 12;
constexpr const Color RADAR_CYLINDER_COLOR = rgba(72, 43, 5, 1);
constexpr const glm::vec3 RADAR_CYLINDER_MODEL_TRANSLATION = MODEL_UP * 0.5f;

constexpr const float RADAR_CONE_HEIGHT = 1.0f;
constexpr const float RADAR_CONE_RAIDUS = 0.3f;
constexpr const size_t RADAR_CONE_RESOLUTION = 12;
constexpr const Color RADAR_CONE_COLOR = rgba(255, 0, 0, 1);
constexpr const glm::vec3 RADAR_CONE_MODEL_POSITION = MODEL_UP * 0.9f;
constexpr const glm::vec3 RADAR_CONE_MODEL_ROTATION = {glm::radians(90.0f), 0.0f, 0.0f};

constexpr const glm::vec3 RADAR_POSITION = 1.5f * MODEL_LEFT + 9.0f * MODEL_BACKWARD + 6.05f * MODEL_UP;

const component::Animation::Callback RADAR_ANIMATION = [](std::shared_ptr<component::Transform> transform,
                                                          std::shared_ptr<GameObject> game_object) {
    (void)game_object;
    constexpr const float ROTATION_SPEED = 2.0f * std::numbers::pi_v<float> / 3.0f;
    transform->rotate(ROTATION_SPEED * Time::getDeltaTime(), UP);
};

constexpr const glm::vec3 ROCK_MODEL_TRANSLATION = ZERO;
constexpr const glm::vec3 ROCK_MODEL_ROTATION = {glm::radians(180.0f), 0.0f, 0.0f};
constexpr const glm::vec3 ROCK_MODEL_SCALE = ONE;

constexpr const float MESSAGE_WIDTH = WORLD_WIDTH * 3.0f / 4.0f;
constexpr const float MESSAGE_HEIGHT = MESSAGE_WIDTH * 9.0f / 16.0f;
constexpr const glm::vec3 MESSAGE_POSITION = UP * 60.0f;

constexpr const Duration HIT_VIGNETTE_DURATION = Duration::milliseconds(800.0f);

#pragma endregion model_settings

#pragma region camera_settings

constexpr const double FOV = 45.0;              // °
constexpr const double PERSPECTIVE_NEAR = 0.1;  // m
constexpr const double PERSPECTIVE_FAR = 300.0; // m

constexpr const glm::vec3 CANNON_CAMERA_OFFSET = 1.0f * MODEL_RIGHT + 4.0f * MODEL_BACKWARD + 1.5f * MODEL_UP;
constexpr const glm::vec3 CANNON_BALL_CAMERA_OFFSET = 2.0f * MODEL_BACKWARD + 0.5f * MODEL_UP + 0.5f * MODEL_RIGHT;

static_assert(PERSPECTIVE_FAR > static_cast<double>(WORLD_WIDTH) * std::numbers::sqrt2,
              "Perspective camera far plan not far enough to see the entire map");

#pragma endregion camera_settings

#pragma region game_contants

constexpr const float SUN_INTENSITY = 3.0f;

constexpr const size_t ROCKS_PER_WORLD_SIDE = 24;
constexpr const float WALL_HEIGHT = 4.5f;
constexpr const float WALL_INSET = 5.0f;

constexpr const float SPAWN_LOCATION_INSET = 20.0f;
constexpr const std::array SPAWN_LOCATIONS = {
    (WORLD_WIDTH / 2.0f - SPAWN_LOCATION_INSET) * NORTH + (WORLD_WIDTH / 2.0f - SPAWN_LOCATION_INSET) * EAST,
    (WORLD_WIDTH / 2.0f - SPAWN_LOCATION_INSET) * SOUTH + (WORLD_WIDTH / 2.0f - SPAWN_LOCATION_INSET) * EAST,
    (WORLD_WIDTH / 2.0f - SPAWN_LOCATION_INSET) * SOUTH + (WORLD_WIDTH / 2.0f - SPAWN_LOCATION_INSET) * WEST,
    (WORLD_WIDTH / 2.0f - SPAWN_LOCATION_INSET) * NORTH + (WORLD_WIDTH / 2.0f - SPAWN_LOCATION_INSET) * WEST,
    ZERO,
};

constexpr const size_t ENEMY_COUNT = 2;

constexpr const float SHIP_MAX_HIT_POINTS = 24'000.0f;
constexpr const float CANNON_BALL_MIN_DAMAGE = 3'000.0f;
constexpr const float CANNON_BALL_MAX_DAMAGE = 15'000.0f;

constexpr const float MAX_EXPLOSION_RAIDUS = 5.0f;            // m
constexpr const float EXPLOSION_RADIUS_EXPANTION_RATE = 8.0f; // m/s
constexpr const Duration EXPLOSION_MIN_HIT_DELAY = Duration::seconds(10.0f);
const component::Animation::Callback EXPLOSION_ANIMATION = [](std::shared_ptr<component::Transform> transform,
                                                              std::shared_ptr<GameObject> game_object) {
    if (Time::paused)
        return;

    const auto scale = transform->getScale();
    auto radius = scale.x; // assume uniform scaling

    radius += EXPLOSION_RADIUS_EXPANTION_RATE * Time::getDeltaTime();
    transform->setScale(radius * ONE);

    if (radius >= MAX_EXPLOSION_RAIDUS)
        EventQueue::post<event::DetachGameObject>(game_object->getId());
};

static_assert(ENEMY_COUNT < SPAWN_LOCATIONS.size(), "not enough spawn location for every enemies");

#pragma endregion game_contants

#pragma region particles_settings

constexpr const Duration CANNON_BALL_SPARK_PARTICLE_SPAWN_INTERVAL = Duration::milliseconds(20.0f);
const auto CANNON_BALL_SPARK_ANIMATION_FACTORY = [] {
    const component::Animation::Callback animation_callback = [last_spawn = Time::now() -
                                                                            CANNON_BALL_SPARK_PARTICLE_SPAWN_INTERVAL](
                                                                  std::shared_ptr<component::Transform> transform,
                                                                  std::shared_ptr<GameObject> game_object) mutable {
        const auto rigid_body = game_object->getComponent<component::RigidBody>().value();
        const auto backward = -getForwardVector(transform->getRotation());
        const auto position = glm::vec3(transform->resolve()[3]) + backward * 0.3f + Random::direction() * 0.05f;

        const auto particle_count = static_cast<size_t>((Time::now() - last_spawn).toSeconds() /
                                                        CANNON_BALL_SPARK_PARTICLE_SPAWN_INTERVAL.toSeconds());
        if (particle_count == 0)
            return;
        last_spawn = Time::now();

        const void *additional_data = event::SpawnParticles::createAdditionalData(backward, rigid_body->getVelocity());
        EventQueue::post<event::SpawnParticles>(event::SpawnParticles::Type::CannonBallSpark, position, particle_count, additional_data);
    };
    return animation_callback;
};

#pragma endregion particles_settings

Application::Application() : should_close_(false), free_view_override_(false)
{
    ProfileScope;

    Time::initialize();
    Random::initialize();

    /******************************************************************************/
    /*                              Window & OpenGL                               */
    /******************************************************************************/

    window_ = std::make_unique<Window>();
    initializeOpenGL();

    /******************************************************************************/
    /*                                   Inputs                                   */
    /******************************************************************************/

    Input::initialize(*window_);

    // clang-format off
    Input::bindMouseButton(Input::Action::UIClick,      GLFW_MOUSE_BUTTON_1);
    Input::bindKey(Input::Action::ToggleFullScreen,     GLFW_KEY_F11       );
    Input::bindKey(Input::Action::ToggleFreeView,       GLFW_KEY_ENTER     );
    Input::bindKey(Input::Action::CycleRenderingStyles, GLFW_KEY_R         );
    Input::bindKey(Input::Action::ToggleDebugMode,      GLFW_KEY_F3        );
    Input::bindKey(Input::Action::DebugMoveTargetNorth, GLFW_KEY_UP        );
    Input::bindKey(Input::Action::DebugMoveTargetEast,  GLFW_KEY_RIGHT     );
    Input::bindKey(Input::Action::DebugMoveTargetSouth, GLFW_KEY_DOWN      );
    Input::bindKey(Input::Action::DebugMoveTargetWest,  GLFW_KEY_LEFT      );
    Input::bindKey(Input::Action::DebugAimAndFire,      GLFW_KEY_F         );
    Input::bindKey(Input::Action::CycleCameras,         GLFW_KEY_V         );
    Input::bindKey(Input::Action::PauseTime,            GLFW_KEY_P         );
    Input::bindKey(Input::Action::RestartGame,          GLFW_KEY_G         );
    Input::bindKey(Input::Action::QuitGame,             GLFW_KEY_ESCAPE    );

    /******************************************************************************/
    /*                                   Shaders                                  */
    /******************************************************************************/
    
    LOG_INFO("compiling shaders");
    
    const std::vector<std::filesystem::path> shared_shader_code = {
        "Maths.frag",
        "SampleEquirect.frag",
        "PBRHelpers.frag",
        "ToneMapping.frag",
    };

    ResourceLoader::load<resource::Shader>("Sky",          "Sky.vert",          "Sky.frag",          resource::Shader::Defines{}, shared_shader_code);
    ResourceLoader::load<resource::Shader>("PBR",          "PBR.vert",          "PBR.frag",          resource::Shader::Defines{
        {std::string_view("MAX_DIRECTIONAL_LIGHTS"), std::optional(std::to_string(component::DirectionalLight::MAX_DIRECTIONAL_LIGHTS))},
    }, shared_shader_code);
    ResourceLoader::load<resource::Shader>("PBR#FLAP",     "PBR.vert",          "PBR.frag",          resource::Shader::Defines{
        {std::string_view("MAX_DIRECTIONAL_LIGHTS"), std::optional(std::to_string(component::DirectionalLight::MAX_DIRECTIONAL_LIGHTS))},
        {std::string_view("FLAP"), std::nullopt},
    }, shared_shader_code);
    ResourceLoader::load<resource::Shader>("WorldColor",   "WorldColor.vert",   "WorldColor.frag"  );
    ResourceLoader::load<resource::Shader>("WorldTexture", "WorldTexture.vert", "WorldTexture.frag");
    ResourceLoader::load<resource::Shader>("Water",        "Water.vert",        "Water.frag", resource::Shader::Defines{
        {std::string_view("MAX_DIRECTIONAL_LIGHTS"), std::optional(std::to_string(component::DirectionalLight::MAX_DIRECTIONAL_LIGHTS))}
    }, shared_shader_code
        ,
        std::optional(std::filesystem::path("WaterTCS.glsl")),
        std::optional(std::filesystem::path("WaterTES.glsl"))
    );
    ResourceLoader::load<resource::Shader>("Particle",     "Particle.vert",     "Particle.frag"    );
    ResourceLoader::load<resource::Shader>("UI",           "UI.vert",           "UI.frag"          );
    
    ResourceLoader::load<resource::ComputeShader>("Particle", "Particle.comp");

    /******************************************************************************/
    /*                                  Textures                                  */
    /******************************************************************************/
    
    LOG_INFO("loading assets");
    
    ResourceLoader::load<resource::Texture>("MissingAlbedo",              "MissingAlbedo.png",              resource::Texture::Type::Albedo                 );
    ResourceLoader::load<resource::Texture>("MissingMetallicRoughness",   "MissingMetallicRoughness.png",   resource::Texture::Type::MetallicRoughness      );
    ResourceLoader::load<resource::Texture>("MissingNormalMap",           "MissingNormalMap.png",           resource::Texture::Type::NormalMap              );
    ResourceLoader::load<resource::Texture>("Sky/SkyBox",                 "Sky/SkyBox.hdr",                 resource::Texture::Type::Albedo,            true);
    ResourceLoader::load<resource::Texture>("Ship/PlayerVariant",         "Ship/SailsRopePlayerAlbedo.png", resource::Texture::Type::Albedo                 );
    ResourceLoader::load<resource::Texture>("Effect/HitVignette",         "Effects/HitVignette.png",        resource::Texture::Type::Albedo                 );
    ResourceLoader::load<resource::Texture>("Message/Victory",            "Messages/Victory.png",           resource::Texture::Type::Albedo                 );
    ResourceLoader::load<resource::Texture>("Message/Defeat",             "Messages/Defeat.png",            resource::Texture::Type::Albedo                 );
    ResourceLoader::load<resource::Texture>("Water/NormalMap1",           "Water/NormalMap1.png",           resource::Texture::Type::NormalMap              );
    ResourceLoader::load<resource::Texture>("Water/NormalMap2",           "Water/NormalMap2.png",           resource::Texture::Type::NormalMap              );
    ResourceLoader::load<resource::Texture>("Water/FoamMap",              "Water/FoamMap.png",              resource::Texture::Type::Noise                  );
    ResourceLoader::load<resource::Texture>("Water/NoiseMap",             "Water/NoiseMap.png",             resource::Texture::Type::Noise                  );

    /******************************************************************************/
    /*                                   Models                                   */
    /******************************************************************************/
    
    LOG_INFO("loading 3d models");

    ResourceLoader::load<resource::Model>("CannonBall",    "CannonBall/CannonBall.gltf"    );
    ResourceLoader::load<resource::Model>("Rocks/1",       "Rocks/Rock1.gltf"              );
    ResourceLoader::load<resource::Model>("Rocks/2",       "Rocks/Rock2.gltf"              );
    ResourceLoader::load<resource::Model>("Rocks/3",       "Rocks/Rock3.gltf"              );
    ResourceLoader::load<resource::Model>("Ship",          "Ship/Ship.gltf"                );
    ResourceLoader::load<resource::Model>("Cannon/Stand",  "CannonStand/CannonStand.gltf"  );
    ResourceLoader::load<resource::Model>("Cannon/Barrel", "CannonBarrel/CannonBarrel.gltf");
    
    ResourceLoader::load<resource::Model>("Effect", generateQuad());
    ResourceLoader::load<resource::Model>("Radar/Cylinder",
        generateCylinder(RADAR_CYLINDER_HEIGHT, RADAR_CYLINDER_RAIDUS, RADAR_CYLINDER_RESOLUTION),
        RADAR_CYLINDER_COLOR
    );
    ResourceLoader::load<resource::Model>("Radar/Cone",
        generateCone(RADAR_CONE_HEIGHT, RADAR_CONE_RAIDUS, RADAR_CONE_RESOLUTION),
        RADAR_CONE_COLOR
    );
    ResourceLoader::load<resource::Model>("Flag", [] {
        constexpr const size_t STRIPS = 16;
        constexpr const float HEIGHT = 1.75f;
        constexpr const float UV_TOP = 0.61816f;
        constexpr const float UV_LEFT = 0.17480f;
        constexpr const float UV_BOTTOM = 0.77689f;
        constexpr const float UV_RIGHT = 0.34326f;
        return generateFlag(STRIPS, component::Flag::WIDTH, HEIGHT, UV_TOP, UV_LEFT, UV_BOTTOM, UV_RIGHT);
    }());
    ResourceLoader::load<resource::Model>("Particle", [] {
        const std::vector<VertexParticle> vertices = {
            {
                .position = {0.0f, 0.5f},
            },
            {
                .position = {-0.5f, 0.0f},
            },
            {
                .position = {0.5f, 0.0f},
            },
            {
                .position = {0.0f, -0.5f},
            },
        };

        const std::vector<IndexType> indices = {
            1, 0, 2,
            1, 2, 3,
        };

        return Mesh{vertices, indices};
    }());
    ResourceLoader::load<resource::Model>("HealBar", generateQuad());
    ResourceLoader::load<resource::Model>("Text", generateQuad());
    ResourceLoader::load<resource::Model>("SkyCube", [] {
        constexpr const float HALF_SIZE = 0.5f;

        const std::vector<VertexDebug> vertices = {
            {{-HALF_SIZE, -HALF_SIZE, -HALF_SIZE}}, {{+HALF_SIZE, -HALF_SIZE, -HALF_SIZE}},
            {{+HALF_SIZE, +HALF_SIZE, -HALF_SIZE}}, {{-HALF_SIZE, +HALF_SIZE, -HALF_SIZE}},
            {{-HALF_SIZE, -HALF_SIZE, +HALF_SIZE}}, {{+HALF_SIZE, -HALF_SIZE, +HALF_SIZE}},
            {{+HALF_SIZE, +HALF_SIZE, +HALF_SIZE}}, {{-HALF_SIZE, +HALF_SIZE, +HALF_SIZE}},
        };

        const std::vector<IndexType> indices = {
            0, 1, 2, 0, 2, 3, // back
            4, 6, 5, 4, 7, 6, // front
            0, 4, 5, 0, 5, 1, // bottom
            3, 2, 6, 3, 6, 7, // top
            1, 5, 6, 1, 6, 2, // right
            0, 3, 7, 0, 7, 4, // left
        };

        return Mesh{vertices, indices};
    }());

    /******************************************************************************/
    /*                               Initialization                               */
    /******************************************************************************/

    LOG_INFO("initializing");

    LOG_DEBUG("cannon_balls initial velocity: {} m/s", INITIAL_CANNON_BALL_VELOCITY);

    resource::Texture::MISSING_ALBEDO             = ResourceLoader::get<resource::Texture>("MissingAlbedo");
    resource::Texture::MISSING_METALLIC_ROUGHNESS = ResourceLoader::get<resource::Texture>("MissingMetallicRoughness");
    resource::Texture::MISSING_NORMAL_MAP         = ResourceLoader::get<resource::Texture>("MissingNormalMap");

    component::Camera3D::initialize({
        ResourceLoader::get<resource::Shader>("PBR"),
        ResourceLoader::get<resource::Shader>("PBR#FLAP"),
        ResourceLoader::get<resource::Shader>("WorldColor"),
        ResourceLoader::get<resource::Shader>("WorldTexture"),
        ResourceLoader::get<resource::Shader>("Water"),
        ResourceLoader::get<resource::Shader>("Particle"),
        ResourceLoader::get<resource::Shader>("Sky"),
    });
    component::DirectionalLight::initialize({
        ResourceLoader::get<resource::Shader>("PBR"),
        ResourceLoader::get<resource::Shader>("PBR#FLAP"),
        ResourceLoader::get<resource::Shader>("Water"),
    });
    ParticleSystem::initialize();

    const resource::Model::MaterialsOverride PLAYER_SHIP_MATERIALS_OVERRIDE = {
        {
            0,
            {
                .albedo_texture = ResourceLoader::get<resource::Texture>("Ship/PlayerVariant"),
                .emissive_color = color::TRANSPARENT,
            },
        },
    };
    const resource::Model::MaterialsOverride ENEMY_SHIP_MATERIALS_OVERRIDE = {};

    /******************************************************************************/
    /*                                   Scene                                    */
    /******************************************************************************/

    std::shared_ptr<GameObject> water;
    [&] {
        scene_root_ = std::make_shared<GameObject>();
        scene_root_->addComponent<component::Transform>();
        scene_root_->addComponent<component::Sky>(ResourceLoader::get<resource::Texture>("Sky/SkyBox"), std::vector{
            std::weak_ptr(ResourceLoader::get<resource::Shader>("PBR")),
            std::weak_ptr(ResourceLoader::get<resource::Shader>("PBR#FLAP")),
            std::weak_ptr(ResourceLoader::get<resource::Shader>("Water")),
        });
        scene_root_->addComponent<component::DirectionalLight>(
            glm::normalize(2.0f * DOWN + WEST + SOUTH),
            color::SUN,
            SUN_INTENSITY);
        Singleton::scene_root = scene_root_;

        // - Free View Camera
        auto perspective_camera = scene_root_->addChild();
        perspective_camera->addComponent<component::Transform>(glm::vec3{5.0f, 5.0f, 5.0f});
        free_view_camera_ = perspective_camera->addComponent<component::Camera3D>(
            component::Camera3D::Perspective{
                .fov = FOV,
                .near = PERSPECTIVE_NEAR,
                .far = PERSPECTIVE_FAR,
            },
            EAST, false);
        free_view_controls_ = perspective_camera->addComponent<component::FreeViewControls>();

        // - Top View Camera
        auto top_view_camera = scene_root_->addChild();
        top_view_camera->addComponent<component::Transform>(UP * 80.0f - NORTH * 1.0f);
        top_view_camera_ = top_view_camera->addComponent<component::Camera3D>(
            component::Camera3D::Orthographic{
                .scale = 100,
                .near = 10.0,
                .far = 100.0,
            },
            glm::normalize(DOWN + NORTH * 0.01f));

        if constexpr (DEBUG_SCENE)
        {
            // Ship
            auto ship = scene_root_->addChild();
            ship->addComponent<component::Transform>();

            auto ship_model = ship->addChild();
            ship_model->addComponent<component::Transform>(SHIP_MODEL_TRANSLATION, SHIP_MODEL_ROTATION, SHIP_MODEL_SCALE);
            ship_model->addComponent<component::ModelInstance>(ResourceLoader::get<resource::Model>("Ship"));

            // Cannon Stand
            auto cannon_stand = scene_root_->addChild();
            cannon_stand->addComponent<component::Transform>(EAST * 10.0f);

            auto cannon_stand_model = cannon_stand->addChild();
            cannon_stand_model->addComponent<component::Transform>(CANNON_STAND_MODEL_TRANSLATION, CANNON_STAND_MODEL_ROTATION, CANNON_STAND_MODEL_SCALE);
            cannon_stand_model->addComponent<component::ModelInstance>(ResourceLoader::get<resource::Model>("Cannon/Stand"));

            // Cannon Barrel
            auto cannon_barrel = scene_root_->addChild();
            cannon_barrel->addComponent<component::Transform>(EAST * 15.0f);

            auto cannon_barrel_model = cannon_barrel->addChild();
            cannon_barrel_model->addComponent<component::Transform>(CANNON_BARREL_MODEL_TRANSLATION, CANNON_BARREL_MODEL_ROTATION, CANNON_BARREL_MODEL_SCALE);
            cannon_barrel_model->addComponent<component::ModelInstance>(ResourceLoader::get<resource::Model>("Cannon/Barrel"));

            // Cannon Ball
            auto cannon_ball = scene_root_->addChild();
            cannon_ball->addComponent<component::Transform>(EAST * 20.0f);

            auto cannon_ball_model = cannon_ball->addChild();
            cannon_ball_model->addComponent<component::Transform>(CANNON_BALL_MODEL_TRANSLATION, CANNON_BALL_MODEL_ROTATION, CANNON_BALL_MODEL_SCALE);
            cannon_ball_model->addComponent<component::ModelInstance>(ResourceLoader::get<resource::Model>("CannonBall"));

            // Rock 1
            auto rock_1 = scene_root_->addChild();
            rock_1->addComponent<component::Transform>(EAST * 30.0f);

            auto rock_1_model = rock_1->addChild();
            rock_1_model->addComponent<component::Transform>(ROCK_MODEL_TRANSLATION, ROCK_MODEL_ROTATION, ROCK_MODEL_SCALE);
            rock_1_model->addComponent<component::ModelInstance>(ResourceLoader::get<resource::Model>("Rocks/1"));

            // Rock 2
            auto rock_2 = scene_root_->addChild();
            rock_2->addComponent<component::Transform>(EAST * 40.0f);

            auto rock_2_model = rock_2->addChild();
            rock_2_model->addComponent<component::Transform>(ROCK_MODEL_TRANSLATION, ROCK_MODEL_ROTATION, ROCK_MODEL_SCALE);
            rock_2_model->addComponent<component::ModelInstance>(ResourceLoader::get<resource::Model>("Rocks/2"));

            // Rock 3
            auto rock_3 = scene_root_->addChild();
            rock_3->addComponent<component::Transform>(EAST * 55.0f);

            auto rock_3_model = rock_3->addChild();
            rock_3_model->addComponent<component::Transform>(ROCK_MODEL_TRANSLATION, ROCK_MODEL_ROTATION, ROCK_MODEL_SCALE);
            rock_3_model->addComponent<component::ModelInstance>(ResourceLoader::get<resource::Model>("Rocks/3"));
        }
        else
        {
            const auto addShip = [&](resource::Model::MaterialsOverride materials_override, std::string_view flag_texture_name) {
                auto ship = scene_root_->addChild();
                auto ship_transform = ship->addComponent<component::Transform>();
                auto ship_collider = ship->addComponent<component::Collider>(SHIP_MODEL_COLLIDER);
                ship_collider->setCollisionResolutionMask({1.0f, 1.0f, 0.0f});
                ship->addComponent<component::RigidBody>(SHIP_MASS);
                                                                                                                    
                auto health_bar = scene_root_->addChild();
                std::weak_ptr health_bar_weak = health_bar;
                auto ship_health = ship->addComponent<component::Health>(
                    SHIP_MAX_HIT_POINTS, [health_bar_weak](std::shared_ptr<GameObject> game_object) {
                        LOG_DEBUG("ship {} sunk", game_object->getId());
                        game_object->visible = false;
                        game_object->active = false;
                        health_bar_weak.lock()->active = false;
                        health_bar_weak.lock()->visible = false;
                        EventQueue::post<event::ShipSunk>(game_object->getId());
                    });
                                                                                                                    
                /* health bar*/
                health_bar->addComponent<component::Transform>();
                health_bar->addComponent<component::HealthBar>(ship_health, ship_transform);
                                                                                                                    
                /* ship model */
                auto ship_model = ship->addChild();
                ship_model->addComponent<component::Transform>(SHIP_MODEL_TRANSLATION, SHIP_MODEL_ROTATION, SHIP_MODEL_SCALE);
                ship_model->addComponent<component::ModelInstance>(ResourceLoader::get<resource::Model>("Ship"), materials_override);                    

                /* flag */
                auto flag = ship->addChild();
                flag->addComponent<component::Transform>(SHIP_FLAG_TRANSLATION, SHIP_FLAG_ROTATION);
                flag->addComponent<component::Flag>(ResourceLoader::get<resource::Texture>(flag_texture_name));
                                                                                                                    
                /* cannon */
                auto cannon = ship->addChild();
                cannon->addComponent<component::Transform>(CANNON_POSITION_IN_SHIP);
                                                                                                                    
                /* - stand model */
                auto cannon_stand_model = cannon->addChild();
                cannon_stand_model->addComponent<component::Transform>(CANNON_STAND_MODEL_TRANSLATION, CANNON_STAND_MODEL_ROTATION, CANNON_STAND_MODEL_SCALE);
                cannon_stand_model->addComponent<component::ModelInstance>(ResourceLoader::get<resource::Model>("Cannon/Stand"));
                                                                                                                    
                /* - barrel container */
                auto barrel_container = cannon->addChild();
                barrel_container->addComponent<component::Transform>(CANNON_BARREL_POSITION_IN_CANNON, CANNON_BARREL_ROTATION_IN_CANNON);
                                                                                                                    
                /* - - barrel */
                auto cannon_barrel = barrel_container->addChild();
                auto cannon_barrel_transform = cannon_barrel->addComponent<component::Transform>();
                cannon_barrel_transform->pointToward(EAST);
                                                                                                                    
                /* - - - barrel model */
                auto cannon_barrel_model = cannon_barrel->addChild();
                cannon_barrel_model->addComponent<component::Transform>(CANNON_BARREL_MODEL_TRANSLATION, CANNON_BARREL_MODEL_ROTATION, CANNON_BARREL_MODEL_SCALE);
                cannon_barrel_model->addComponent<component::ModelInstance>(ResourceLoader::get<resource::Model>("Cannon/Barrel"));
                                                                                                                    
                /* radar */
                auto radar = ship->addChild();
                radar->addComponent<component::Transform>(RADAR_POSITION);
                radar->addComponent<component::Animation>(RADAR_ANIMATION);
                                                                                                                    
                /* - cylinder */
                auto radar_cylinder = radar->addChild();
                radar_cylinder->addComponent<component::Transform>(RADAR_CYLINDER_MODEL_TRANSLATION);
                radar_cylinder->addComponent<component::ModelInstance>(ResourceLoader::get<resource::Model>("Radar/Cylinder"));
                                                                                                                    
                /* - cone */
                auto radar_cone = radar->addChild();
                radar_cone->addComponent<component::Transform>(RADAR_CONE_MODEL_POSITION, RADAR_CONE_MODEL_ROTATION);
                radar_cone->addComponent<component::ModelInstance>(ResourceLoader::get<resource::Model>("Radar/Cone"));
            
                return std::make_tuple(ship, health_bar, cannon, cannon_barrel_transform);
            };

            // - World border
            const std::array rocks = {
                ResourceLoader::get<resource::Model>("Rocks/1"),
                ResourceLoader::get<resource::Model>("Rocks/2"),
                ResourceLoader::get<resource::Model>("Rocks/3"),
            };

            for (size_t i = 0; i < 4; ++i)
            {
                bool along_north = (i & 2);
                bool potitive    = (i & 1);

                auto wall = scene_root_->addChild();
                wall->addComponent<component::Transform>((WORLD_WIDTH / 2.0f - WALL_INSET) * (potitive ? 1.0f : -1.0f) * (along_north ? NORTH : EAST));
                wall->addComponent<component::Collider>(component::Collider::AABB{
                    .half_size = WALL_HEIGHT / 2.0f * UP
                               + WORLD_WIDTH / 2.0f * (along_north ? EAST : NORTH)
                               + 0.5f * (along_north ? NORTH : EAST),
                    .center = WALL_HEIGHT / 2.0f * UP,
                });
                wall->addComponent<component::RigidBody>();

                for (size_t j = 0; j < ROCKS_PER_WORLD_SIDE; ++j)
                {
                    const float c1 = WORLD_WIDTH * (static_cast<float>(j) / static_cast<float>(ROCKS_PER_WORLD_SIDE - 1) - 0.5f);
                    const float c2 = WORLD_WIDTH / 2.0f;
                    const float east  = (potitive ? 1.0f : -1.0f) * (along_north ? c1 : c2);
                    const float north = (potitive ? 1.0f : -1.0f) * (along_north ? c2 : c1);

                    std::shared_ptr<resource::Model> random_rock_model =
                        Random::range<decltype(random_rock_model)>(rocks);
                    const float angle = Random::random(0.0f, glm::radians(359.9f));

                    auto rock = scene_root_->addChild();
                    rock->addComponent<component::Transform>(east * EAST + north * NORTH, angle * UP);

                    auto rock_model = rock->addChild();
                    rock_model->addComponent<component::Transform>(ROCK_MODEL_TRANSLATION, ROCK_MODEL_ROTATION, ROCK_MODEL_SCALE);
                    rock_model->addComponent<component::ModelInstance>(random_rock_model);
                }
            }
        
            // - Player
            auto [player_ship, player_health_bar, player_cannon, player_cannon_barrel_transform] = addShip(PLAYER_SHIP_MATERIALS_OVERRIDE, "Ship/PlayerVariant");
            ships_and_health_bars_.push_back({player_ship, player_health_bar});
            player_id_ = player_ship->getId();

            auto cannon_camera = player_cannon->addChild();
            cannon_camera->addComponent<component::Transform>(CANNON_CAMERA_OFFSET);
            cannon_camera_ = cannon_camera->addComponent<component::Camera3D>(
                component::Camera3D::Perspective{
                    .fov = FOV,
                    .near = PERSPECTIVE_NEAR,
                    .far = PERSPECTIVE_FAR,
                },
                EAST);
        
            player_cannon->addComponent<component::CannonPlayerController>(player_cannon_barrel_transform, cannon_camera_);
            player_ship->addComponent<component::ShipPlayerController>();

            // - Enemies
            for (size_t i = 0; i < ENEMY_COUNT; ++i)
            {
                auto [enemy_ship, enemy_health_bar, enemy_cannon, enemy_cannon_barrel_transform] = addShip(ENEMY_SHIP_MATERIALS_OVERRIDE, ENEMY_SHIP_FLAG);
                ships_and_health_bars_.push_back({enemy_ship, enemy_health_bar});

                auto enemy_ship_target = scene_root_->addChild();
                auto enemy_ship_target_transform = enemy_ship_target->addComponent<component::Transform>();
                enemy_ship_target
                    ->addComponent<component::Collider>(component::Collider::AABB{
                        .half_size = 0.5f * ONE,
                        .center = ZERO,
                    })
                    ->disable();

                enemy_ship->addComponent<component::ShipAIController>(enemy_ship_target_transform);
                enemy_cannon->addComponent<component::CannonAIController>(enemy_cannon_barrel_transform);
            }
        }

        // - Water
        water = scene_root_->addChild();
        water->addComponent<component::Transform>(glm::vec3{}, glm::vec3{}, glm::vec3{1.0f, 1.0f, 1.0f});
        water->addComponent<component::Collider>(
            component::Collider::AABB{
                .half_size = {WORLD_WIDTH / 2.0f, WORLD_WIDTH / 2.0f, 0.5f},
                .center = {0.0f, 0.0f, -0.5f},
            },
            true);
        water->addComponent<component::Water>();

        if constexpr (!DEBUG_SCENE)
        {
            // - Victory message
            auto victory_message = scene_root_->addChild();
            victory_message->addComponent<component::Transform>(MESSAGE_POSITION);
            victory_message->addComponent<component::Text>(MESSAGE_WIDTH, MESSAGE_HEIGHT, ResourceLoader::get<resource::Texture>("Message/Victory"));
            victory_message_ = victory_message;

            // - Defeat message
            auto defeat_message = scene_root_->addChild();
            defeat_message->addComponent<component::Transform>(MESSAGE_POSITION);
            defeat_message->addComponent<component::Text>(MESSAGE_WIDTH, MESSAGE_HEIGHT, ResourceLoader::get<resource::Texture>("Message/Defeat"));
            defeat_message_ = defeat_message;
        }
    }();

    main_view_ = View::Top;
    if constexpr (DEBUG_SCENE)
    {
        free_view_override_ = true;
        free_view_controls_.lock()->active = true;
        window_->captureMouse();
    }
    else
    {
        Singleton::view = main_view_;
    }

    scene_root_->initialize();
    restart();

    /******************************************************************************/
    /*                              Events Callbacks                              */
    /******************************************************************************/

    const auto water_id = water->getId();
    EventQueue::registerCallback<event::Fire>([this, water_id](const event::Fire &event) {
        if (glm::length(event.initial_velocity) < EPSILON)
        {
            LOG_DEBUG("fire aborted: no initial velocity");
            return;
        }

        auto cannon_ball = scene_root_->addChild();
        std::weak_ptr<GameObject> weak_cannon_ball = cannon_ball;
        to_detach_on_restart_[cannon_ball->getId()] = weak_cannon_ball;

        cannon_ball->addComponent<component::Transform>(event.position)
            ->pointToward(glm::normalize(event.initial_velocity));

        auto cannon_ball_collider = cannon_ball->addComponent<component::Collider>(CANNON_BALL_COLLIDER);
        const auto shooter_id = event.shooter;
        cannon_ball_collider->addCollisionCallback(
            [this, weak_cannon_ball, water_id, shooter_id](const GameObjectId id) {
                if (id == shooter_id)
                {
                    return false;
                }

                auto cannon_ball_non_weak = weak_cannon_ball.lock();
                const auto cannon_ball_id = cannon_ball_non_weak->getId();

                if (last_cannon_ball_camera_.has_value() &&
                    cannon_ball_id == last_cannon_ball_camera_.value().lock()->getOwner()->getParent().value()->getId())
                {
                    last_cannon_ball_camera_ = std::nullopt;
                    if (main_view_ == View::CannonBall)
                    {
                        main_view_ = View::Cannon;
                        updateActiveView();
                    }
                }

                const auto cannon_ball_transform = cannon_ball_non_weak->getComponent<component::Transform>().value();
                const auto cannon_ball_position  = glm::vec3(cannon_ball_transform->resolve()[3]);

                if (id == water_id)
                {
                    constexpr const size_t PLOOF_PARTICLE_COUNT = 500;
                    EventQueue::post<event::SpawnParticles>(event::SpawnParticles::Type::WaterSplash, cannon_ball_position, PLOOF_PARTICLE_COUNT);
                }
                else
                {
                    std::shared_ptr<component::Transform> transform =
                        cannon_ball_non_weak->getComponent<component::Transform>().value();

                    auto explosion = scene_root_->addChild();
                    explosion->addComponent<component::Transform>(glm::vec3(transform->resolve()[3]));
                    explosion->addComponent<component::Collider>(component::Collider::Sphere{
                        .center = ZERO,
                        .radius = 0.5f,
                    });
                    explosion->addComponent<component::Attack>(CANNON_BALL_MIN_DAMAGE, CANNON_BALL_MAX_DAMAGE,
                                                               EXPLOSION_MIN_HIT_DELAY);
                    explosion->addComponent<component::Animation>(EXPLOSION_ANIMATION);
                    explosion->initialize();
                    to_detach_on_restart_[explosion->getId()] = explosion;

                    constexpr const size_t EXPLOSION_PARTICLE_COUNT = 2500;
                    EventQueue::post<event::SpawnParticles>(event::SpawnParticles::Type::Explosion, cannon_ball_position, EXPLOSION_PARTICLE_COUNT);
                }

                std::erase_if(to_detach_on_restart_, [cannon_ball_id](auto pair) {
                    auto &[_id, _] = pair;
                    return _id == cannon_ball_id;
                });

                return true;
            });

        auto rigid_body = cannon_ball->addComponent<component::RigidBody>(CANNON_BALL_MASS);
        rigid_body->setVelocity(event.initial_velocity);

        auto cannon_ball_model = cannon_ball->addChild();
        cannon_ball_model->addComponent<component::Transform>(CANNON_BALL_MODEL_TRANSLATION, CANNON_BALL_MODEL_ROTATION,
                                                              CANNON_BALL_MODEL_SCALE);
        cannon_ball_model->addComponent<component::ModelInstance>(ResourceLoader::get<resource::Model>("CannonBall"));
        cannon_ball_model->addComponent<component::Animation>(
            [](std::shared_ptr<component::Transform> transform, std::shared_ptr<GameObject> game_object) {
                (void)game_object;

                if (Singleton::view == View::Top)
                {
                    transform->setScale(CANNON_BALL_TOP_VIEW_SCALE_FACTOR * CANNON_BALL_MODEL_SCALE);
                }
                else
                {
                    transform->setScale(CANNON_BALL_MODEL_SCALE);
                }
            });

        cannon_ball->addComponent<component::Animation>(CANNON_BALL_SPARK_ANIMATION_FACTORY());

        if (event.shooter == player_id_)
        {
            auto cannon_ball_camera = cannon_ball->addChild();
            cannon_ball_camera->addComponent<component::Transform>(CANNON_BALL_CAMERA_OFFSET);
            last_cannon_ball_camera_ = {cannon_ball_camera->addComponent<component::Camera3D>(
                component::Camera3D::Perspective{
                    .fov = FOV,
                    .near = PERSPECTIVE_NEAR,
                    .far = PERSPECTIVE_FAR,
                },
                NORTH)};

            if (main_view_ == View::Cannon)
            {
                main_view_ = View::CannonBall;
                updateActiveView();
            }
        }

        cannon_ball->initialize();

        if (event.shooter == player_id_)
        {
            last_cannon_ball_camera_.value().lock()->lookToward(glm::normalize(event.initial_velocity));
        }
    });

    EventQueue::registerCallback<event::DetachGameObject>([this](const event::DetachGameObject &event) {
        auto game_object_option = Singleton::scene_root.lock()->getGameObject(event.game_object_id);
        if (!game_object_option.has_value())
            return;

        auto game_object = game_object_option.value();
        const auto game_object_id = game_object->getId();
        game_object->detach();

        std::erase_if(to_detach_on_restart_, [game_object_id](auto pair) {
            auto &[_id, _] = pair;
            return _id == game_object_id;
        });
    });

    EventQueue::registerCallback<event::ShipSunk>([this](const event::ShipSunk &event) {
        (void)event;

        size_t enemy_sunk_count = 0;
        for (const auto pair : ships_and_health_bars_)
        {
            const auto ship = std::get<0>(pair).lock();

            if (ship->active)
                continue;

            if (ship->getId() == player_id_)
            {
                EventQueue::post<event::GameEnd>(false);
                return;
            }
            else
                enemy_sunk_count += 1;
        }

        if (enemy_sunk_count == ships_and_health_bars_.size() - 1)
        {
            EventQueue::post<event::GameEnd>(true);
        }
    });

    EventQueue::registerCallback<event::GameEnd>([this](const event::GameEnd &event) {
        if (event.victory)
        {
            LOG_DEBUG("victory");
            victory_message_.lock()->visible = true;
        }
        else
        {
            LOG_DEBUG("defeat");
            defeat_message_.lock()->visible = true;
        }

        main_view_ = View::Top;
        updateActiveView();
    });

    EventQueue::registerCallback<event::DamageTaken>([this](const event::DamageTaken &event) {
        if (event.game_object_id != player_id_)
            return;

        component::Camera3D::displayEffect(ResourceLoader::get<resource::Texture>("Effect/HitVignette"), HIT_VIGNETTE_DURATION);
        component::Camera3D::shake(HIT_VIGNETTE_DURATION);
    });

    EventQueue::registerCallback<event::SpawnParticles>([](const event::SpawnParticles &event) {
        std::vector<Particle> particles(event.count);

        switch (event.type)
        {
        case event::SpawnParticles::Type::Explosion: {
            constexpr const Color    EXPLOSION_PARTICLE_INNER_COLOR  = rgba(220, 192, 70, 0.9);
            constexpr const Color    EXPLOSION_PARTICLE_OUTTER_COLOR = rgba(252, 55, 29, 0.86);
            constexpr const Duration EXPLOSION_PARTICLE_MAX_LIFETIME = Duration::seconds(MAX_EXPLOSION_RAIDUS / EXPLOSION_RADIUS_EXPANTION_RATE);
            constexpr const float    EXPLOSION_PARTICLE_MAX_VELOCITY = 10.0f; // m/s

            for (auto &particle : particles)
            {
                const float t = std::sqrt(Random::random(0.0f, 1.0f));

                particle.position = event.position;
                particle.velocity = t * EXPLOSION_PARTICLE_MAX_VELOCITY * Random::direction();
                particle.life     = EXPLOSION_PARTICLE_MAX_LIFETIME.toSeconds();
                particle.color    = glm::mix(EXPLOSION_PARTICLE_INNER_COLOR, EXPLOSION_PARTICLE_OUTTER_COLOR, t);
                particle.scale    = {0.5f, 0.5f}; 
                particle.is_subject_to_gravity = false;
            }
        }
        break;
        case event::SpawnParticles::Type::Smoke: {
            constexpr const float    SMOKE_PARTICLE_SPREAD       = glm::radians(5.0f);
            constexpr const Duration SMOKE_PARTICLE_MIN_LIFETIME = Duration::milliseconds(400.0f);
            constexpr const Duration SMOKE_PARTICLE_MAX_LIFETIME = Duration::milliseconds(1500.0f);

            for (auto &particle : particles)
            {
                particle.position = event.position + 0.2f * Random::direction();
                particle.velocity = Random::direction(UP, SMOKE_PARTICLE_SPREAD);
                particle.color    = Color(Random::random(0.0f, 0.4f) * ONE, Random::random(0.3f, 0.8f));
                particle.life     = Random::random(SMOKE_PARTICLE_MIN_LIFETIME.toSeconds(), SMOKE_PARTICLE_MAX_LIFETIME.toSeconds());
                particle.scale    = {0.1f, 0.1f};
                particle.is_subject_to_gravity = false;
            }
        }
        break;
        case event::SpawnParticles::Type::FoamTrail: {
            constexpr const Color FOAM_PARTICLE_COLOR  = color::WHITE;
            constexpr const float FOAM_PARTICLE_SPREAD = 2.0f; // m

            for (auto &particle : particles)
            {
                particle.position = event.position + Random::direction() * Random::random(0.0f, FOAM_PARTICLE_SPREAD);
                particle.velocity = ZERO;
                particle.color    = FOAM_PARTICLE_COLOR;
                particle.life     = Random::random(0.05f, 0.5f);
                particle.scale    = {0.3f, 0.3f};
                particle.is_subject_to_gravity = true;
            }
        }
        break;
        case event::SpawnParticles::Type::WaterSplash: {
            constexpr const float    PLOOF_PARTICLE_SPAWN_RADIUS = 2.0f; // m
            constexpr const Color    PLOOF_PARTICLE_INNER_COLOR  = rgba(140, 188, 236, 0.81);
            constexpr const Color    PLOOF_PARTICLE_OUTTER_COLOR = rgba(0, 102, 204, 0.6);
            constexpr const Duration PLOOF_PARTICLE_MAX_LIFETIME = Duration::seconds(3.0f);
            constexpr const float    PLOOF_PARTICLE_VERTICALITY  = 10.0f;
            constexpr const float    PLOOF_PARTICLE_SPREAD       = glm::radians(3.0f);
            constexpr const float    PLOOF_PARTICLE_VELOCITY     = 10.0f; // m/s

            for (auto &particle : particles)
            {
                const float radius = Random::random(0.0f, PLOOF_PARTICLE_SPAWN_RADIUS);
                const float angle  = Random::radians();

                const auto offset = radius * (std::cos(angle) * EAST + std::sin(angle) * NORTH);

                particle.position = event.position + offset;
                particle.velocity =
                    Random::random(PLOOF_PARTICLE_VELOCITY / 5.0f, PLOOF_PARTICLE_VELOCITY) *
                    Random::direction(glm::normalize(offset + UP * PLOOF_PARTICLE_VERTICALITY), PLOOF_PARTICLE_SPREAD);
                particle.life     = PLOOF_PARTICLE_MAX_LIFETIME.toSeconds();
                particle.color    = glm::mix(PLOOF_PARTICLE_INNER_COLOR, PLOOF_PARTICLE_OUTTER_COLOR,
                                          std::sqrt(radius / PLOOF_PARTICLE_SPAWN_RADIUS));
                particle.scale    = {1.0f, 1.0f};
                particle.is_subject_to_gravity = true;
            }
        }
        break;
        case event::SpawnParticles::Type::CannonBallSpark: {
            constexpr const float    CANNON_BALL_SPARK_PARTICLE_SPREAD       = glm::radians(20.0f);
            constexpr const Duration CANNON_BALL_SPARK_PARTICLE_MAX_LIFETIME = Duration::milliseconds(20.0f);
            constexpr const Color    CANNON_BALL_SPARK_PARTICLE_COLOR_1      = rgba(252, 233, 62, 1);
            constexpr const Color    CANNON_BALL_SPARK_PARTICLE_COLOR_2      = rgba(255, 29, 29, 1);

            const auto [backward, rigid_body_velocity] = event::SpawnParticles::getAdditionalData<glm::vec3, glm::vec3>(event.additional_data);

            for (auto &particle : particles)
            {
                particle.position = event.position;
                particle.velocity = Random::direction(backward, CANNON_BALL_SPARK_PARTICLE_SPREAD) + rigid_body_velocity;
                particle.color    = glm::mix(CANNON_BALL_SPARK_PARTICLE_COLOR_1, CANNON_BALL_SPARK_PARTICLE_COLOR_2, Random::random(0.0f, 1.0f));
                particle.life     = CANNON_BALL_SPARK_PARTICLE_MAX_LIFETIME.toSeconds();
                particle.scale    = {0.1f, 0.1f};
                particle.is_subject_to_gravity = true;
            }
        }
        break;
        }

        ParticleSystem::addParticles(particles);
        if (event.additional_data != nullptr)
        {
            std::free(const_cast<void*>(event.additional_data));
        }
    });

    // clang-format on

    Singleton::game_loaded = true;
    LOG_INFO("ready");
}

void Application::initializeOpenGL()
{
    ProfileScope;

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Depth Test
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Back Faces Culling
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
}

void Application::run()
{
    while (!(window_->shouldClose() || should_close_))
    {
        const float delta_time = clock_.tick().toSeconds();
        if (delta_time > 1.0f)
        {
            continue;
        }

        update(delta_time);
        render();

        window_->endFrame();

        ProfilingEndFrame;
    }
}

void Application::updateActiveView()
{
    if (free_view_override_)
    {
        Singleton::active_camera = free_view_camera_;
        Singleton::view = View::FreeCamera;
        return;
    }

    Singleton::view = main_view_;
    switch (main_view_)
    {
    case View::FreeCamera:
        LOG_WARNING("This should not be reachable: main_view_ should never be View::FreeCamera");
        break;
    case View::Top:
        Singleton::active_camera = top_view_camera_;
        break;
    case View::Cannon:
        Singleton::active_camera = cannon_camera_;
        break;
    case View::CannonBall:
        Singleton::active_camera = last_cannon_ball_camera_.value();
        break;
    }
}

void Application::update(float delta_time)
{
    ProfileScope;

    Time::update(delta_time);
    Input::update();

    if (Input::getState(Input::Action::ToggleFullScreen) == Input::State::JustReleased)
    {
        window_->toggleFullScreen();
    }
    if (Input::getState(Input::Action::ToggleFreeView) == Input::State::JustReleased && !DEBUG_SCENE)
    {
        if (!free_view_override_)
        {
            free_view_override_ = true;
            free_view_controls_.lock()->active = true;
            window_->captureMouse();
            LOG_INFO("free view mode enabled");
        }
        else
        {
            free_view_override_ = false;
            free_view_controls_.lock()->active = false;
            window_->releaseMouse();
            LOG_INFO("free view mode disabled");
        }
    }
    if (Input::getState(Input::Action::CycleRenderingStyles) == Input::State::JustReleased)
    {
        switch (Singleton::rendering_style)
        {
        case RenderingStyle::OpaquePolygon: {
            LOG_INFO("switched to wireframe rendering");
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDisable(GL_POLYGON_OFFSET_FILL);
            Singleton::rendering_style = RenderingStyle::Wireframe;
        }
        break;
        case RenderingStyle::Wireframe: {
            LOG_INFO("switched to wireframe with hidden lines removal rendering");
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(1.0f, 1.0f);
            Singleton::rendering_style = RenderingStyle::WireframeWithHiddenLinesRemoval;
        }
        break;
        case RenderingStyle::WireframeWithHiddenLinesRemoval: {
            LOG_INFO("switched to opaque polygon rendering");
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDisable(GL_POLYGON_OFFSET_FILL);
            Singleton::rendering_style = RenderingStyle::OpaquePolygon;
        }
        break;
        }
    }
    if (Input::getState(Input::Action::ToggleDebugMode) == Input::State::JustReleased)
    {
        Singleton::debug = !Singleton::debug;
        if (Singleton::debug)
        {
            LOG_INFO("debug mode enabled");
        }
        else
        {
            LOG_INFO("debug mode disabled");
        }
    }
    if (Input::getState(Input::Action::CycleCameras) == Input::State::JustReleased && !DEBUG_SCENE)
    {
        switch (main_view_)
        {
        case View::FreeCamera:
            LOG_WARNING("This should not be reachable: main_view_ should never be View::FreeCamera");
            break;
        case View::Top:
            main_view_ = last_cannon_ball_camera_.has_value() ? View::CannonBall : View::Cannon;
            break;
        case View::Cannon:
            [[fallthrough]];
        case View::CannonBall:
            main_view_ = View::Top;
            break;
        }
    }
    if (Input::getState(Input::Action::PauseTime) == Input::State::JustReleased)
    {
        Time::paused = !Time::paused;
    }
    if (Input::getState(Input::Action::RestartGame) == Input::State::JustReleased && !DEBUG_SCENE)
    {
        restart();
    }
    if (Input::getState(Input::Action::QuitGame) == Input::State::JustReleased)
    {
        should_close_ = true;
    }

    updateActiveView();

    EventQueue::processAll();

    component::Camera3D::updateEffect();
    scene_root_->update();
    Physics::update();
    ParticleSystem::update();
}

void Application::render() const
{
    ProfileScope;
    ProfileScopeGPU("Application::render");

    window_->startRendering();

    auto camera = Singleton::active_camera.lock();
    camera->bind();
    component::DirectionalLight::beginRender();

    if (Singleton::rendering_style == RenderingStyle::WireframeWithHiddenLinesRemoval)
    {
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        renderPass(camera);

        component::DirectionalLight::endRender();

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        renderPass(camera);
    }
    else
    {
        renderPass(camera);
        component::DirectionalLight::endRender();
    }
}

void Application::renderPass(std::shared_ptr<component::Camera3D> camera) const
{
    ProfileScope;
    ProfileScopeGPU("Application::renderPass");

    static const std::vector<std::weak_ptr<resource::Shader>> shaders_for_frame_buffer_mapping = {
        std::weak_ptr(ResourceLoader::get<resource::Shader>("Water")),
    };

    window_->bindFrameBuffer();
    scene_root_->render();

    window_->mapFrameBuffer(shaders_for_frame_buffer_mapping);
    scene_root_->renderDefered();

    ParticleSystem::render();
    camera->renderEffect();
}

void Application::restart()
{
    ProfileScope;

    if constexpr (!DEBUG_SCENE)
    {
        // hide messages
        victory_message_.lock()->visible = false;
        defeat_message_.lock()->visible = false;

        // destroy cannon balls & explosions
        for (auto &[_, game_object] : to_detach_on_restart_)
        {
            game_object.lock()->detach();
        }
        to_detach_on_restart_.clear();

        auto spawn_locations = SPAWN_LOCATIONS | std::ranges::to<std::vector>();

        for (auto pair : ships_and_health_bars_)
        {
            auto [weak_ship, weak_health_bar] = pair;

            auto ship = weak_ship.lock();
            auto health_bar = weak_health_bar.lock();

            // reactivate ships
            ship->active = true;
            ship->visible = true;
            health_bar->active = true;
            health_bar->visible = true;

            // replace ships
            const auto ship_position = Random::pop(spawn_locations);
            auto ship_transform = ship->getComponent<component::Transform>().value();
            ship_transform->setPosition(ship_position);
            auto ship_rigid_body = ship->getComponent<component::RigidBody>().value();
            ship_rigid_body->reset();

            auto ship_player_controller_option = ship->getComponent<component::ShipPlayerController>();
            if (ship_player_controller_option.has_value())
            {
                ship_player_controller_option.value()->stop();
            }

            // refill ship health
            ship->getComponent<component::Health>().value()->heal();
        }
    }

    const auto [framebuffer_width, framebuffer_height] = window_->getFramebufferSize();
    EventQueue::post<event::WindowResized>(framebuffer_width, framebuffer_height);
}
