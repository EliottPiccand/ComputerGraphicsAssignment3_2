#include "Components/Text.h"

#include "Resources/Model.h"
#include "Resources/ResourceLoader.h"
#include "Resources/Shader.h"
#include "Utils/Profiling.h"

using namespace component;

Text::Text(float width, float height, std::weak_ptr<resource::Texture> texture)
    : width_(width), height_(height), texture_(texture)
{
}

void Text::render(glm::mat4 &transform) const
{
    ProfileScope;
    ProfileScopeGPU("Text::render");

    static std::weak_ptr weak_shader = ResourceLoader::getAsset<resource::Shader>("WorldTexture");
    static std::weak_ptr model =
        ResourceLoader::getOrFactoryLoad<resource::Model>("Text", [] { return std::make_tuple(generateQuad()); });

    auto texture = texture_.lock();

    auto shader = weak_shader.lock();
    shader->bind();

    shader->setUniform("u_Model", transform * glm::scale(glm::vec3{width_, height_, 1.0f}));
    texture->bind(GL_TEXTURE0, shader, "u_Texture");
    model.lock()->draw(shader, {{0, {{resource::Texture::Type::Albedo, texture}}}});
}
