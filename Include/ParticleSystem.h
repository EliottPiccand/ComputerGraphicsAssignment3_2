#pragma once

#include <vector>

#include <Lib/glm.h>

#include "Utils/Color.h"
#include "Utils/Time.h"

struct Particle
{
    glm::vec3 position;
    glm::vec3 velocity;
    Color color;
    Duration max_lifetime;
    Instant lifetime_start;
    bool subject_to_gravity;
};

class ParticleSystem
{
  public:
    static void addParticles(const std::vector<Particle> &particles);

    static void update(float delta_time);
    static void render();

  private:
    static inline std::vector<Particle> particles_;
};
