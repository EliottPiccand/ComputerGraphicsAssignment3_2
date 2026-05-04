#include "Utils/Random.h"

void Random::initialize()
{
    generator_ = std::mt19937(random_device_());
}

float Random::random(float min, float max)
{
    std::uniform_real_distribution<float> distribution(min, max);
    return distribution(generator_);
}

int Random::randint(int min, int max)
{
    std::uniform_int_distribution<int> distribution(min, max);
    return distribution(generator_);
}

float Random::radians()
{
    return Random::random(0.0f, glm::radians(359.9f));
};

glm::vec3 Random::direction(const glm::vec3 &along, float spread)
{
    float angle = Random::random(0.0f, spread);
    float azimuth = Random::random(0.0f, 2.0f * glm::pi<float>());

    glm::vec3 right = glm::normalize(glm::cross(along, glm::abs(along.y) < 0.9f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0)));
    glm::vec3 up = glm::cross(right, along);

    glm::vec3 cone_dir = glm::cos(angle) * along + glm::sin(angle) * (glm::cos(azimuth) * right + glm::sin(azimuth) * up);

    return glm::normalize(cone_dir);
}

glm::vec3 Random::direction()
{
    const float z = Random::random(-1.0f, 1.0f);
    const float azimuth = Random::random(0.0f, 2.0f * glm::pi<float>());
    const float radius = glm::sqrt(glm::max(0.0f, 1.0f - z * z));

    return glm::vec3(radius * glm::cos(azimuth), radius * glm::sin(azimuth), z);
}
