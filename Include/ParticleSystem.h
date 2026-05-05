#pragma once

#include <cstdint>
#include <span>

#include <Lib/OpenGL.h>
#include <Lib/glm.h>

#include "Utils/Color.h"

struct alignas(16) Particle
{
    alignas(16) glm::vec3 position;
    float life;
    alignas(16) glm::vec3 velocity;
    int32_t is_subject_to_gravity;
    alignas(16) Color color;
    alignas(16) glm::vec2 scale;
};

class ParticleSystem
{
  public:
    static void addParticles(std::span<const Particle> particles);

    static void initialize();
    static void update();
    static void render();

  private:
    static inline GLuint ssbo_input_;
    static inline GLuint ssbo_output_;
    static inline GLuint ssbo_count_;
    static inline uint32_t particle_count_;
};
