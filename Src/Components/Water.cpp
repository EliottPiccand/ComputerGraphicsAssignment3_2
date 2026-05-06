#include "Components/Water.h"

#include <memory>

#include "Mesh/Mesh.h"
#include "Resources/ResourceLoader.h"
#include "Resources/Shader.h"
#include "Resources/Texture.h"
#include "Utils/Constants.h"
#include "Utils/Profiling.h"
#include "Utils/Time.h"

using namespace component;

Water::Water()
{
    ProfileScope;

    constexpr const size_t QUADS_PER_SIDE = static_cast<size_t>(WORLD_WIDTH);

    const auto [vertices, indices] = generateQuadPlane(QUADS_PER_SIDE, QUADS_PER_SIDE, WORLD_WIDTH / static_cast<float>(QUADS_PER_SIDE));

    glGenVertexArrays(1, &vertex_array_);
    glBindVertexArray(vertex_array_);

    glGenBuffers(1, &vertex_buffer_);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(VertexWater)), vertices.data(),
                 GL_STATIC_DRAW);

    VertexWater::setupVertexArray();

    glGenBuffers(1, &index_buffer_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(IndexType)), indices.data(),
                 GL_STATIC_DRAW);
    index_count_ = static_cast<GLsizei>(indices.size());
}

void Water::renderDefered(glm::mat4 &transform) const
{
    ProfileScope;
    ProfileScopeGPU("Water::renderDefered");

    static std::weak_ptr weak_shader = ResourceLoader::get<resource::Shader>("Water");

    static std::weak_ptr normal_map_1 = ResourceLoader::get<resource::Texture>("Water/NormalMap1");
    static std::weak_ptr normal_map_2 = ResourceLoader::get<resource::Texture>("Water/NormalMap2");
    static std::weak_ptr foam_map     = ResourceLoader::get<resource::Texture>("Water/FoamMap");
    static std::weak_ptr noise_map    = ResourceLoader::get<resource::Texture>("Water/NoiseMap");
    
    auto shader = weak_shader.lock();
    shader->bind();
    shader->setUniform("u_Model", transform);
    shader->setUniform("u_ModelInverse", glm::inverse(transform));

    shader->setUniform("u_LightDirection",             glm::normalize(2.0f * DOWN + WEST + SOUTH));
    shader->setUniform("u_WaterSurfaceColor",          glm::vec4(0.465f, 0.797f, 0.991f, 1.0f));
    shader->setUniform("u_WaterRefractionColor",       glm::vec4(0.003f, 0.599f, 0.812f, 1.0f));
    shader->setUniform("u_SsrSettings",                glm::vec4(0.5f, 20.0f, 10.0f, 20.0f));
    shader->setUniform("u_NormalMapScroll",            glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    shader->setUniform("u_NormalMapScrollSpeed",       glm::vec2(0.01f, 0.01f));
    shader->setUniform("u_RefractionDistortionFactor", 0.04f);
    shader->setUniform("u_RefractionHeightFactor",     2.5f);
    shader->setUniform("u_RefractionDistanceFactor",   15.0f);
    shader->setUniform("u_DepthSofteningDistance",     0.5f);
    shader->setUniform("u_FoamHeightStart",            0.8f);
    shader->setUniform("u_FoamFadeDistance",           0.4f);
    shader->setUniform("u_FoamTiling",                 2.0f);
    shader->setUniform("u_FoamAngleExponent",          80.0f);
    shader->setUniform("u_FoamBrightness",             4.0f);
    shader->setUniform("u_Roughness",                  0.08f);
    shader->setUniform("u_Reflectance",                0.55f);
    shader->setUniform("u_SpecIntensity",              125.0f);
    shader->setUniform("u_TessellationFactor",         7.0f);
    shader->setUniform("u_DampeningFactor",            5.0f);
    shader->setUniform("u_Time",                       Time::elapsed().toSeconds());

    normal_map_1.lock()->bind(resource::Texture::NORMAL_MAP_SLOT, shader, "u_WaterNormalMap1");
    normal_map_2.lock()->bind(resource::Texture::NORMAL_MAP_SLOT_2, shader, "u_WaterNormalMap2");
    foam_map.lock()->bind(resource::Texture::NOISE_MAP_SLOT, shader, "u_WaterFoamMap");
    noise_map.lock()->bind(resource::Texture::NOISE_MAP_SLOT_2, shader, "u_WaterNoiseMap");

    glBindVertexArray(vertex_array_);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer_);

    glPatchParameteri(GL_PATCH_VERTICES, 3);
    glDrawElements(GL_PATCHES, index_count_, GL_INDEX_TYPE, nullptr);
}
