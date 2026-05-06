#include "Components/Text.h"

#include "Resources/Model.h"
#include "Resources/ResourceLoader.h"
#include "Resources/Shader.h"
#include "Resources/Texture.h"
#include "Utils/Profiling.h"

using namespace component;

Text::Text(float width, float height, std::weak_ptr<resource::Texture> texture)
    : width_(width), height_(height), texture_(texture)
{
}

void Text::renderDefered(glm::mat4 &transform) const
{
    ProfileScope;
    ProfileScopeGPU("Text::renderDefered");

    static std::weak_ptr weak_shader = ResourceLoader::get<resource::Shader>("WorldTexture");
    static std::weak_ptr model = ResourceLoader::get<resource::Model>("Text");

    auto texture = texture_.lock();

    auto shader = weak_shader.lock();
    shader->bind();

    shader->setUniform("u_Model", transform * glm::scale(glm::vec3{width_, height_, 1.0f}));
    texture->bind(resource::Texture::BASE_COLOR_SLOT, shader, "u_Texture");
    model.lock()->draw(shader);
}
