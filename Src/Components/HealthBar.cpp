#include "Components/HealthBar.h"

#include <tuple>

#include <Lib/OpenGL.h>
#include <Lib/glm.h>

#include "Resources/Model.h"
#include "Resources/ResourceLoader.h"
#include "Resources/Shader.h"
#include "Utils/Color.h"
#include "Utils/Constants.h"
#include "Utils/Profiling.h"

using namespace component;

HealthBar::HealthBar(std::weak_ptr<Health> health, std::weak_ptr<Transform> follow_target)
    : health_(health), follow_target_(follow_target)
{
}

void HealthBar::initialize()
{
    ProfileScope;

    GET_COMPONENT(Transform, transform_, HealthBar);
}

void HealthBar::update()
{
    ProfileScope;

    constexpr const float HEALTH_BAR_HEIGHT = 50.0f;

    auto transform = transform_.lock();
    auto follow_target_transform = follow_target_.lock();
    auto follow_target_position = follow_target_transform->getPosition();

    transform->setPosition(follow_target_position - UP * glm::dot(follow_target_position, UP) + UP * HEALTH_BAR_HEIGHT);
}

void HealthBar::render(glm::mat4 &transform) const
{
    ProfileScope;
    ProfileScopeGPU("HealthBar::render");

    constexpr const float HEALTH_BAR_WIDTH = 15.0f;
    constexpr const float HEALTH_BAR_HEIGHT = 1.0f;
    constexpr const float HEALTH_BAR_BORDER_WIDTH = 0.5f;

    static std::weak_ptr weak_shader = ResourceLoader::getAsset<resource::Shader>("WorldColor");
    static std::weak_ptr model = ResourceLoader::getOrFactoryLoad<resource::Model>("HealBar", []{
        return std::make_tuple(generateQuad());
    });

    auto shader = weak_shader.lock();
    shader->bind();

    // Background
    constexpr glm::vec3 BACKGROUND_SCALE = {HEALTH_BAR_WIDTH + 2.0f * HEALTH_BAR_BORDER_WIDTH, HEALTH_BAR_HEIGHT + 2.0f * HEALTH_BAR_BORDER_WIDTH, 1.0f};

    shader->setUniform("u_Model", transform * glm::scale(BACKGROUND_SCALE));
    shader->setUniform("u_Color", color::BLACK);
    model.lock()->draw(shader);

    // Forground
    const auto t = health_.lock()->getRemainingHealthRatio();
    const glm::vec3 FORGROUND_SCALE = {HEALTH_BAR_WIDTH * t, HEALTH_BAR_HEIGHT, 1.0f};    

    shader->setUniform("u_Model", transform * glm::translate(WEST * (1.0f - t) * HEALTH_BAR_WIDTH / 2.0f + UP) * glm::scale(FORGROUND_SCALE));
    shader->setUniform("u_Color", t < 0.5f ? glm::mix(color::RED, color::YELLOW, t * 2.0f) : glm::mix(color::YELLOW, color::GREEN, t * 2.0f - 1.0f));
    model.lock()->draw(shader);
}
