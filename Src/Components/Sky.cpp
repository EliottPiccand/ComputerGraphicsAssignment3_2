#include "Components/Sky.h"

#include <array>
#include <cmath>
#include <stdexcept>

#include <Lib/OpenGL.h>
#include <Lib/glm.h>

#include "Resources/Model.h"
#include "Resources/ResourceLoader.h"
#include "Resources/Shader.h"
#include "Utils/Log.h"
#include "Utils/Profiling.h"

using namespace component;

namespace
{

GLuint createCubemap(GLsizei size, GLint internal_format, int mip_levels)
{
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture_id);

    for (int mip = 0; mip < mip_levels; ++mip)
    {
        const GLsizei mip_size = std::max<GLsizei>(1, size >> mip);
        for (GLuint face = 0; face < 6; ++face)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, mip, internal_format, mip_size, mip_size, 0, GL_RGB,
                         GL_FLOAT, nullptr);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
                    mip_levels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    return texture_id;
}

GLuint createBRDFLut(GLsizei size)
{
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, size, size, 0, GL_RG, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);
    return texture_id;
}

void checkFramebufferComplete(const char *stage)
{
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        LOG_ERROR("framebuffer is incomplete during {}", stage);
        throw std::runtime_error("IBL framebuffer setup failed");
    }
}

std::array<glm::mat4, 6> captureViews()
{
    return {
        glm::lookAt(glm::vec3(0.0f), glm::vec3(+1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, +1.0f, 0.0f), glm::vec3(0.0f, 0.0f, +1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, +1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    };
}

void drawSkyCube(const std::shared_ptr<resource::Model> &cube_model, const std::shared_ptr<resource::Shader> &shader)
{
    shader->setUniform("u_Model", glm::mat4(1.0f));
    cube_model->draw(shader);
}

} // namespace


Sky::Sky(std::weak_ptr<resource::Texture> texture) : texture_(texture)
{
}

void Sky::initialize()
{
    if (ibl_ready_)
    {
        return;
    }

    ProfileScope;
    ProfileScopeGPU("Sky::initialize");

    auto texture = texture_.lock();
    if (texture == nullptr)
    {
        LOG_ERROR("Sky::initialize: missing environment texture");
        throw std::runtime_error("missing sky texture");
    }

    static std::shared_ptr<resource::Model> cube_model = ResourceLoader::get<resource::Model>("SkyCube");
    static std::weak_ptr weak_equirect_to_cubemap = ResourceLoader::get<resource::Shader>("EquirectToCubemap");
    static std::weak_ptr weak_irradiance = ResourceLoader::get<resource::Shader>("IrradianceConvolution");
    static std::weak_ptr weak_prefilter = ResourceLoader::get<resource::Shader>("PrefilterEnv");
    static std::weak_ptr weak_brdf = ResourceLoader::get<resource::Shader>("BrdfIntegration");

    auto equirect_to_cubemap = weak_equirect_to_cubemap.lock();
    auto irradiance_shader = weak_irradiance.lock();
    auto prefilter_shader = weak_prefilter.lock();
    auto brdf_shader = weak_brdf.lock();

    const int environment_mip_count = static_cast<int>(std::floor(std::log2(ENVIRONMENT_MAP_SIZE))) + 1;

    environment_cubemap_ = createCubemap(ENVIRONMENT_MAP_SIZE, GL_RGB16F, environment_mip_count);
    irradiance_cubemap_ = createCubemap(IRRADIANCE_MAP_SIZE, GL_RGB16F, 1);
    prefilter_cubemap_ = createCubemap(PREFILTER_MAP_SIZE, GL_RGB16F, PREFILTER_MIP_LEVEL_COUNT);
    brdf_lut_ = createBRDFLut(BRDF_LUT_SIZE);

    GLuint capture_fbo;
    GLuint capture_rbo;
    glGenFramebuffers(1, &capture_fbo);
    glGenRenderbuffers(1, &capture_rbo);

    glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo);
    glBindRenderbuffer(GL_RENDERBUFFER, capture_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, ENVIRONMENT_MAP_SIZE, ENVIRONMENT_MAP_SIZE);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, capture_rbo);

    GLint previous_viewport[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, previous_viewport);

    SCOPE_DISABLE(GL_CULL_FACE);

    const glm::mat4 capture_projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    const auto capture_views = captureViews();

    // Convert equirectangular map to cubemap.
    {
        equirect_to_cubemap->bind();
        equirect_to_cubemap->setUniform("u_Projection", capture_projection);
        texture->bind(GL_TEXTURE0, equirect_to_cubemap, "u_EquirectangularMap");

        glViewport(0, 0, ENVIRONMENT_MAP_SIZE, ENVIRONMENT_MAP_SIZE);
        glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo);

        for (GLuint face = 0; face < 6; ++face)
        {
            equirect_to_cubemap->setUniform("u_View", capture_views[static_cast<size_t>(face)]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                                   environment_cubemap_, 0);
            checkFramebufferComplete("equirectangular-to-cubemap");
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            drawSkyCube(cube_model, equirect_to_cubemap);
        }

        glBindTexture(GL_TEXTURE_CUBE_MAP, environment_cubemap_);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    }

    // Convolve cubemap to diffuse irradiance map.
    {
        glBindRenderbuffer(GL_RENDERBUFFER, capture_rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, IRRADIANCE_MAP_SIZE, IRRADIANCE_MAP_SIZE);

        irradiance_shader->bind();
        irradiance_shader->setUniform("u_Projection", capture_projection);
        irradiance_shader->bindTexture("u_EnvironmentMap", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, environment_cubemap_);

        glViewport(0, 0, IRRADIANCE_MAP_SIZE, IRRADIANCE_MAP_SIZE);
        glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo);

        for (GLuint face = 0; face < 6; ++face)
        {
            irradiance_shader->setUniform("u_View", capture_views[static_cast<size_t>(face)]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                                   irradiance_cubemap_, 0);
            checkFramebufferComplete("irradiance convolution");
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            drawSkyCube(cube_model, irradiance_shader);
        }
    }

    // Generate prefiltered environment map for roughness-dependent specular reflections.
    {
        prefilter_shader->bind();
        prefilter_shader->setUniform("u_Projection", capture_projection);
        prefilter_shader->setUniform("u_EnvironmentResolution", static_cast<float>(ENVIRONMENT_MAP_SIZE));
        prefilter_shader->bindTexture("u_EnvironmentMap", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, environment_cubemap_);

        glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo);
        for (int mip = 0; mip < PREFILTER_MIP_LEVEL_COUNT; ++mip)
        {
            const GLsizei mip_size = std::max<GLsizei>(1, PREFILTER_MAP_SIZE >> mip);

            glBindRenderbuffer(GL_RENDERBUFFER, capture_rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mip_size, mip_size);
            glViewport(0, 0, mip_size, mip_size);

            const float roughness = static_cast<float>(mip) / static_cast<float>(PREFILTER_MIP_LEVEL_COUNT - 1);
            prefilter_shader->setUniform("u_Roughness", roughness);

            for (GLuint face = 0; face < 6; ++face)
            {
                prefilter_shader->setUniform("u_View", capture_views[static_cast<size_t>(face)]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                                       prefilter_cubemap_, mip);
                checkFramebufferComplete("prefilter map generation");
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                drawSkyCube(cube_model, prefilter_shader);
            }
        }
    }

    // Integrate BRDF lookup texture.
    {
        glBindRenderbuffer(GL_RENDERBUFFER, capture_rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, BRDF_LUT_SIZE, BRDF_LUT_SIZE);

        glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdf_lut_, 0);
        checkFramebufferComplete("BRDF LUT integration");

        glViewport(0, 0, BRDF_LUT_SIZE, BRDF_LUT_SIZE);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GLuint lut_vao;
        glGenVertexArrays(1, &lut_vao);
        glBindVertexArray(lut_vao);

        brdf_shader->bind();
        glDrawArrays(GL_TRIANGLES, 0, 3);

        glBindVertexArray(0);
        glDeleteVertexArrays(1, &lut_vao);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glViewport(previous_viewport[0], previous_viewport[1], previous_viewport[2], previous_viewport[3]);

    glDeleteRenderbuffers(1, &capture_rbo);
    glDeleteFramebuffers(1, &capture_fbo);

    ibl_ready_ = true;
}

void Sky::render(glm::mat4 &transform) const
{
    ProfileScope;
    ProfileScopeGPU("Sky::render");

    (void)transform;

    static std::weak_ptr weak_shader = ResourceLoader::get<resource::Shader>("Sky");
    static std::shared_ptr<resource::Model> cube_model = ResourceLoader::get<resource::Model>("SkyCube");

    SCOPE_DEPTH_MASK(GL_FALSE);
    SCOPE_DISABLE(GL_DEPTH_TEST);
    SCOPE_DISABLE(GL_CULL_FACE);

    auto shader = weak_shader.lock();
    shader->bind();

    texture_.lock()->bind(GL_TEXTURE0, shader, "u_EnvironmentMap");
    cube_model->draw(shader);
}

void Sky::bindIBL(const std::shared_ptr<resource::Shader> &shader)
{
    if (!ibl_ready_)
    {
        shader->setUniform("u_UseIBL", false);
        return;
    }

    shader->setUniform("u_UseIBL", true);

    const float max_reflection_lod =
        std::max(0.0f, std::floor(std::log2(static_cast<float>(ENVIRONMENT_MAP_SIZE))));
    shader->setUniform("u_MaxReflectionLod", max_reflection_lod);

    shader->bindTexture("u_IrradianceMap", 10);
    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_cubemap_);

    shader->bindTexture("u_PrefilterMap", 11);
    glActiveTexture(GL_TEXTURE11);
    glBindTexture(GL_TEXTURE_CUBE_MAP, environment_cubemap_);

    shader->bindTexture("u_BrdfLut", 12);
    glActiveTexture(GL_TEXTURE12);
    glBindTexture(GL_TEXTURE_2D, brdf_lut_);
}
