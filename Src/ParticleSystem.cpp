#include "ParticleSystem.h"

#include <utility>

#include "Lib/OpenGL.h"
#include "Resources/ComputeShader.h"
#include "Resources/Model.h"
#include "Resources/ResourceLoader.h"
#include "Resources/Shader.h"
#include "Utils/Constants.h"
#include "Utils/Log.h"
#include "Utils/Profiling.h"

constexpr const uint32_t MAX_PARTICLES = 100'000;

void ParticleSystem::addParticles(std::span<const Particle> particles)
{
    ProfileScope;

    uint32_t count = static_cast<uint32_t>(particles.size());
    if (particle_count_ + count > MAX_PARTICLES)
    {
        LOG_WARNING("maximum particle count excedded ({}/{}) ! discarding exceding particles", count + particle_count_,
                    MAX_PARTICLES);

        count = MAX_PARTICLES - particle_count_;
        particles = particles.first(count);
    }

    const size_t byte_offset = particle_count_ * sizeof(Particle);
    const size_t byte_size = count * sizeof(Particle);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_input_);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, static_cast<GLsizei>(byte_offset), static_cast<GLsizei>(byte_size),
                    particles.data());
    particle_count_ += count;
}

void ParticleSystem::initialize()
{
    ProfileScope;

    LOG_DEBUG("initializing particle system");

    glGenBuffers(1, &ssbo_input_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_input_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_PARTICLES * sizeof(Particle), nullptr, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &ssbo_output_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_output_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_PARTICLES * sizeof(Particle), nullptr, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &ssbo_count_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_count_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW);
    particle_count_ = 0;
    glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &particle_count_);
}

void ParticleSystem::update(float delta_time)
{
    ProfileScope;

    constexpr const auto GRAVITY_ACCELERATION = GRAVITY * DOWN;

    static std::weak_ptr weak_compute_shader = ResourceLoader::get<resource::ComputeShader>("Particle");
    auto compute_shader = weak_compute_shader.lock();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_input_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_output_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_count_);

    compute_shader->bind();
    compute_shader->setUniform("u_DeltaTime", delta_time);
    compute_shader->setUniform("u_Gravity", GRAVITY_ACCELERATION);
    compute_shader->setUniform("u_ParticlesCount", particle_count_);

    uint32_t zero = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_count_);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &zero);

    uint32_t groups = (particle_count_ + 63) / 64;
    glDispatchCompute(groups, 1, 1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    particle_count_ = 0;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_count_);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(uint32_t), &particle_count_);

    std::swap(ssbo_input_, ssbo_output_);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_input_);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_output_);
}

void ParticleSystem::render()
{
    ProfileScope;
    ProfileScopeGPU("ParticleSystem::render");

    static std::weak_ptr weak_shader = ResourceLoader::get<resource::Shader>("Particle");
    static std::weak_ptr model = ResourceLoader::get<resource::Model>("Particle");

    auto shader = weak_shader.lock();
    shader->bind();
    shader->setUniform("u_Model", glm::mat4(1.0f));

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_input_);

    SCOPE_DEPTH_MASK(GL_FALSE);

    model.lock()->drawInstanced(shader, static_cast<size_t>(particle_count_));
}
