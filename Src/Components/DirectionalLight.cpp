#include "Components/DirectionalLight.h"

#include "Utils/Profiling.h"


component::DirectionalLight::DirectionalLight(glm::vec3 direction, Color color, float intensity)
    : direction_(direction), color_(color), intensity_(intensity)
{
}

component::DirectionalLight::~DirectionalLight()
{
}

void component::DirectionalLight::initialize(std::vector<std::shared_ptr<resource::Shader>> shaders)
{
    for (auto shader : shaders)
    {
        shaders_.push_back(shader);
    }
}

void component::DirectionalLight::beginRender()
{
    for (auto weak_shader : shaders_)
    {
        auto shader = weak_shader.lock();

        shader->bind();
        shader->setUniform("u_AmbientColor", glm::vec3(AMBIENT_COLOR));
    }
    lights_drawn_on_this_frame_ = 0;
}

void component::DirectionalLight::endRender()
{
    for (auto weak_shader : shaders_)
    {
        auto shader = weak_shader.lock();

        shader->bind();
        shader->setUniform("u_DirectionalLightCount", static_cast<int32_t>(lights_drawn_on_this_frame_));
    }
}

void component::DirectionalLight::render(glm::mat4 &transform) const
{
    ProfileScope;
    ProfileScopeGPU("DirectionalLight::render");

    (void)transform;

    const auto light_index = lights_drawn_on_this_frame_;
    lights_drawn_on_this_frame_ += 1;

    for (auto weak_shader : shaders_)
    {
        auto shader = weak_shader.lock();

        shader->bind();
        shader->setUniformArrayElement("u_DirectionalLights[{}].direction", light_index, direction_);
        shader->setUniformArrayElement("u_DirectionalLights[{}].color", light_index, intensity_ * glm::vec3(color_));
    }
}
