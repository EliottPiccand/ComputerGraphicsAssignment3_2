#include "Components/ModelInstance.h"

#include <cassert>

#include "Resources/ResourceLoader.h"
#include "Resources/Shader.h"
#include "Utils/Profiling.h"

using namespace component;

ModelInstance::ModelInstance(std::shared_ptr<resource::Model> model,
                             resource::Model::MaterialsOverride materials_override)
    : model_(model), materials_override_(materials_override)
{
}

void ModelInstance::render(glm::mat4 &transform) const
{
    ProfileScope;
    ProfileScopeGPU("ModelInstance::render");

    static std::weak_ptr weak_shader = ResourceLoader::get<resource::Shader>("PBR");

    auto shader = weak_shader.lock();

    shader->bind();
    shader->setUniform("u_Model", transform);
    shader->setUniform("u_ModelNormal", glm::transpose(glm::inverse(glm::mat3(transform))));
    model_->draw(shader, materials_override_);
}
