#include "Components/Flag.h"

#include "Resources/Model.h"
#include "Resources/ResourceLoader.h"
#include "Resources/Shader.h"
#include "Utils/Profiling.h"

using namespace component;

Flag::Flag(std::weak_ptr<resource::Texture> texture) : animation_time_(0.0f), texture_(texture)
{
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

    constexpr const size_t STRIPS = 16;
    constexpr const float WIDTH = 3.0f;
    constexpr const float HEIGHT = 1.75f;

    constexpr const float UV_TOP = 0.61816f;
    constexpr const float UV_LEFT = 0.17480f;
    constexpr const float UV_BOTTOM = 0.77689f;
    constexpr const float UV_RIGHT = 0.34326f;

    static std::weak_ptr weak_shader = ResourceLoader::getAsset<resource::Shader>("PBR#FLAP");
    static std::weak_ptr model = ResourceLoader::getOrFactoryLoad<resource::Model>("Flag", [] {
        return std::make_tuple(generateFlag(STRIPS, WIDTH, HEIGHT, UV_TOP, UV_LEFT, UV_BOTTOM, UV_RIGHT));
    });

    auto shader = weak_shader.lock();
    shader->bind();
    shader->setUniform("u_Model", transform);
    shader->setUniform("u_Time", animation_time_);
    shader->setUniform("u_Width", WIDTH);

    model.lock()->draw(shader, {
                                   {0,
                                    {
                                        {resource::Texture::Type::Albedo, texture_.lock()},
                                    }},
                               });
}
