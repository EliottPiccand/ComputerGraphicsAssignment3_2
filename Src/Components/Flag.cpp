#include "Components/Flag.h"

#include "Resources/ResourceLoader.h"
#include "Resources/Shader.h"
#include "Utils/Profiling.h"

using namespace component;

Flag::Flag(std::weak_ptr<resource::Texture> texture) : animation_time_(0.0f)
{
    materials_override_ = {
        {
            0,
            {
                .albedo_texture = texture,
            },
        },
    };
}

void Flag::update()
{
    ProfileScope;

    animation_time_ += Time::getDeltaTime();
}

void Flag::render(glm::mat4 &transform) const
{
    ProfileScope;
    ProfileScopeGPU("Flag::render");

    static std::weak_ptr weak_shader = ResourceLoader::get<resource::Shader>("PBR#FLAP");
    static std::weak_ptr model = ResourceLoader::get<resource::Model>("Flag");

    auto shader = weak_shader.lock();
    shader->bind();
    shader->setUniform("u_Model", transform);
    shader->setUniform("u_Time", animation_time_);
    shader->setUniform("u_Width", WIDTH);

    model.lock()->draw(shader, materials_override_);
}
