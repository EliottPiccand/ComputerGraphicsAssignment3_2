#include "Components/Sky.h"

#include <Lib/OpenGL.h>
#include <Lib/glm.h>
#include <Lib/stb.h>

#include "Resources/Model.h"
#include "Resources/ResourceLoader.h"
#include "Resources/Shader.h"
#include "Resources/Texture.h"
#include "Utils/Profiling.h"

using namespace component;

Sky::Sky(std::weak_ptr<resource::Texture> texture, std::vector<std::weak_ptr<resource::Shader>> shaders)
    : shaders_(shaders), texture_(texture)
{
}

void Sky::render(glm::mat4 &transform) const
{
    ProfileScope;
    ProfileScopeGPU("Sky::render");

    (void)transform;

    static std::weak_ptr weak_sky_shader = ResourceLoader::get<resource::Shader>("Sky");
    static std::shared_ptr<resource::Model> cube_model = ResourceLoader::get<resource::Model>("SkyCube");

    SCOPE_DEPTH_MASK(GL_FALSE);
    SCOPE_DISABLE(GL_DEPTH_TEST);
    SCOPE_DISABLE(GL_CULL_FACE);

    auto texture = texture_.lock();

    auto sky_shader = weak_sky_shader.lock();
    sky_shader->bind();

    texture->bind(resource::Texture::ENVIRONMENT_MAP_SLOT, sky_shader, "u_EnvironmentMap");
    cube_model->draw(sky_shader);

    for (auto weak_shader : shaders_)
    {
        auto shader = weak_shader.lock();
        shader->bind();
        texture->bind(resource::Texture::ENVIRONMENT_MAP_SLOT, shader, "u_EnvironmentMap");
    }
}
