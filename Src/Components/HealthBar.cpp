#include "Components/HealthBar.h"

#include <Lib/OpenGL.h>
#include <Lib/glm.h>

#include "Utils/Color.h"
#include "Utils/Constants.h"

using namespace component;

HealthBar::HealthBar(std::weak_ptr<Health> health, std::weak_ptr<Transform> follow_target)
    : health_(health), follow_target_(follow_target)
{
}

void HealthBar::initialize()
{
    GET_COMPONENT(Transform, transform_, HealthBar);
}

void HealthBar::update(float delta_time)
{
    constexpr const float HEALTH_BAR_HEIGHT = 50.0f;

    (void)delta_time;

    auto transform = transform_.lock();
    auto follow_target_transform = follow_target_.lock();
    auto follow_target_position = follow_target_transform->getPosition();

    transform->setPosition(follow_target_position - UP * glm::dot(follow_target_position, UP) + UP * HEALTH_BAR_HEIGHT);
}

bool HealthBar::render() const
{
    constexpr const float HEALTH_BAR_WIDTH = 16.0f;
    constexpr const float HEALTH_BAR_HEIGHT = 2.0f;
    constexpr const float HEALTH_BAR_BORDER_WIDTH = 0.5f;

    // Background

    constexpr const glm::vec3 BACKGROUND_TOP_LEFT     = HEALTH_BAR_WIDTH / 2.0f * WEST + HEALTH_BAR_HEIGHT / 2.0f * NORTH;
    constexpr const glm::vec3 BACKGROUND_TOP_RIGHT    = HEALTH_BAR_WIDTH / 2.0f * EAST + HEALTH_BAR_HEIGHT / 2.0f * NORTH;
    constexpr const glm::vec3 BACKGROUND_BOTTOM_RIGHT = HEALTH_BAR_WIDTH / 2.0f * EAST + HEALTH_BAR_HEIGHT / 2.0f * SOUTH;
    constexpr const glm::vec3 BACKGROUND_BOTTOM_LEFT  = HEALTH_BAR_WIDTH / 2.0f * WEST + HEALTH_BAR_HEIGHT / 2.0f * SOUTH;

    glMaterialfv(GL_FRONT, GL_AMBIENT, color::MATERIAL_BLACK);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, color::MATERIAL_BLACK);

    glBegin(GL_QUADS);
        glVertex3f(_v3(BACKGROUND_TOP_RIGHT));
        glVertex3f(_v3(BACKGROUND_TOP_LEFT));
        glVertex3f(_v3(BACKGROUND_BOTTOM_LEFT));
        glVertex3f(_v3(BACKGROUND_BOTTOM_RIGHT));
    glEnd();

    // Forground
    glPushMatrix();

    constexpr const auto Z_OFFSET = UP * 1.0f;
    glTranslatef(_v3(Z_OFFSET));
    
    constexpr const glm::vec3 FORGROUND_TOP_LEFT     = (HEALTH_BAR_WIDTH / 2.0f - HEALTH_BAR_BORDER_WIDTH) * WEST + (HEALTH_BAR_HEIGHT / 2.0f - HEALTH_BAR_BORDER_WIDTH) * NORTH;
    constexpr const glm::vec3 FORGROUND_BOTTOM_LEFT  = (HEALTH_BAR_WIDTH / 2.0f - HEALTH_BAR_BORDER_WIDTH) * WEST + (HEALTH_BAR_HEIGHT / 2.0f - HEALTH_BAR_BORDER_WIDTH) * SOUTH;
    const auto t = health_.lock()->getRemainingHealthRatio();
    const glm::vec3 FORGROUND_TOP_RIGHT    = (HEALTH_BAR_WIDTH - 2.0f * HEALTH_BAR_BORDER_WIDTH) * (t - 0.5f) * EAST + (HEALTH_BAR_HEIGHT / 2.0f - HEALTH_BAR_BORDER_WIDTH) * NORTH;
    const glm::vec3 FORGROUND_BOTTOM_RIGHT = (HEALTH_BAR_WIDTH - 2.0f * HEALTH_BAR_BORDER_WIDTH) * (t - 0.5f) * EAST + (HEALTH_BAR_HEIGHT / 2.0f - HEALTH_BAR_BORDER_WIDTH) * SOUTH;

    const auto color = glm::mix(color::RED, color::GREEN, t);
    const GLfloat material[] = {_v4(color)};

    glMaterialfv(GL_FRONT, GL_AMBIENT, material);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, material);

    glBegin(GL_QUADS);
        glVertex3f(_v3(FORGROUND_TOP_RIGHT));
        glVertex3f(_v3(FORGROUND_TOP_LEFT));
        glVertex3f(_v3(FORGROUND_BOTTOM_LEFT));
        glVertex3f(_v3(FORGROUND_BOTTOM_RIGHT));
    glEnd();

    glPopMatrix();

    return false;
}
