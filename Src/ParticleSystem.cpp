#include "ParticleSystem.h"

#include <Lib/OpenGL.h>

#include "Utils/Constants.h"

void ParticleSystem::addParticles(const std::vector<Particle> &particles)
{
    particles_.append_range(particles);
}

void ParticleSystem::update(float delta_time)
{
    constexpr const auto acceleration = GRAVITY * DOWN;

    for (auto &particle : particles_)
    {
        if (particle.subject_to_gravity)
        {
            particle.velocity += acceleration * delta_time;
        }
        particle.position += particle.velocity * delta_time;
    }

    const auto instant_now = now();
    std::erase_if(particles_,
                  [instant_now](const auto &p) { return instant_now - p.lifetime_start >= p.max_lifetime; });
}

void ParticleSystem::render()
{
    glPointSize(5.0f);

    // TODO: use intanciated rendering with shaders

    glPushAttrib(GL_ENABLE_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glBegin(GL_POINTS);
    for (const auto &particle : particles_)
    {
        glColor4f(_v4(particle.color));
        glVertex3f(_v3(particle.position));
    }
    glEnd();

    glPopAttrib();
}
