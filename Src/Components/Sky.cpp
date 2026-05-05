#include "Components/Sky.h"

#include <array>
#include <cmath>
#include <stdexcept>
#include <filesystem>

#include <Lib/OpenGL.h>
#include <Lib/glm.h>
#include <Lib/stb.h>

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

void saveCubemapFaceToFile(GLuint cubemap, GLenum face, GLsizei size, int mip, const std::string &filename)
{
    std::vector<float> data(size * size * 3);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
    glGetTexImage(face, mip, GL_RGB, GL_FLOAT, data.data());
    
    // Convert float to byte for PNG
    std::vector<uint8_t> byte_data(size * size * 3);
    for (size_t i = 0; i < data.size(); ++i)
    {
        byte_data[i] = static_cast<uint8_t>(glm::clamp(data[i], 0.0f, 1.0f) * 255.0f);
    }
    
    stbi_write_png(filename.c_str(), size, size, 3, byte_data.data(), size * 3);
    LOG_DEBUG("Saved cubemap face to {}", filename);
}

void saveBRDFLutToFile(GLuint texture, GLsizei size, const std::string &filename)
{
    std::vector<float> data(size * size * 2);
    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_FLOAT, data.data());
    
    // Convert RG float to RGB byte for visualization (R in red channel, G in green channel)
    std::vector<uint8_t> byte_data(size * size * 3);
    for (size_t i = 0; i < size * size; ++i)
    {
        byte_data[i * 3 + 0] = static_cast<uint8_t>(glm::clamp(data[i * 2 + 0], 0.0f, 1.0f) * 255.0f);
        byte_data[i * 3 + 1] = static_cast<uint8_t>(glm::clamp(data[i * 2 + 1], 0.0f, 1.0f) * 255.0f);
        byte_data[i * 3 + 2] = 0; // B channel unused
    }
    
    stbi_write_png(filename.c_str(), size, size, 3, byte_data.data(), size * 3);
    LOG_DEBUG("Saved BRDF LUT to {}", filename);
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

    LOG_INFO("generating sky reflection data");

    LOG_TRACE("Sky::initialize: Starting IBL initialization");

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
        LOG_TRACE("Sky::initialize: Converting equirectangular to cubemap");
        equirect_to_cubemap->bind();
        equirect_to_cubemap->setUniform("u_Projection", capture_projection);
        
        LOG_TRACE("Sky::initialize: Binding HDR texture to slot 0");
        // IMPORTANT: Bind texture manually to ensure it's actually bound to GPU, not just uniform
        equirect_to_cubemap->bindTexture("u_EquirectangularMap", 0);  // Set uniform
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture->getID());  // Actually bind to GPU
        LOG_TRACE("Sky::initialize: Texture manually bound");

        glViewport(0, 0, ENVIRONMENT_MAP_SIZE, ENVIRONMENT_MAP_SIZE);
        glBindFramebuffer(GL_FRAMEBUFFER, capture_fbo);
        
        // DEBUG: Test if rendering works at all - clear to white
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

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
        LOG_TRACE("Sky::initialize: Equirectangular conversion complete");
    }

    // Convolve cubemap to diffuse irradiance map.
    {
        LOG_TRACE("Sky::initialize: Starting irradiance convolution");
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
        LOG_TRACE("Sky::initialize: Irradiance convolution complete");
    }

    // Generate prefiltered environment map for roughness-dependent specular reflections.
    {
        LOG_TRACE("Sky::initialize: Starting prefilter generation");
        prefilter_shader->bind();
        
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            LOG_ERROR("Sky::initialize: GL error after prefilter bind: {}", err);
        }
        
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
                
                err = glGetError();
                if (err != GL_NO_ERROR) {
                    LOG_ERROR("Sky::initialize: GL error at mip {} face {}: {}", mip, face, err);
                }
                
                checkFramebufferComplete("prefilter map generation");
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                drawSkyCube(cube_model, prefilter_shader);
            }
        }
        
        err = glGetError();
        if (err != GL_NO_ERROR) {
            LOG_ERROR("Sky::initialize: GL error after prefilter generation: {}", err);
        }
        
        LOG_TRACE("Sky::initialize: Prefilter generation complete");
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
    LOG_TRACE("Sky::initialize: IBL initialization complete, ibl_ready_=true");

    // DEBUG: Save IBL textures to disk
    {
        std::filesystem::path debug_dir = std::filesystem::current_path() / "debug_ibl";
        std::filesystem::create_directories(debug_dir);
        LOG_INFO("Saving IBL textures to {}", debug_dir.string());
        
        // Save irradiance cubemap (single mip level)
        const char *face_names[] = {"PX", "NX", "PY", "NY", "PZ", "NZ"};
        GLenum faces[] = {GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
                         GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
                         GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z};
        
        LOG_DEBUG("Saving irradiance map ({} faces, {} px each)", 6, IRRADIANCE_MAP_SIZE);
        for (int i = 0; i < 6; ++i)
        {
            std::string filename = (debug_dir / ("irradiance_" + std::string(face_names[i]) + ".png")).string();
            saveCubemapFaceToFile(irradiance_cubemap_, faces[i], IRRADIANCE_MAP_SIZE, 0, filename);
        }
        
        LOG_DEBUG("Saving prefilter map ({} mips, {} faces each, starting at {} px)", 
                  PREFILTER_MIP_LEVEL_COUNT, 6, PREFILTER_MAP_SIZE);
        for (int mip = 0; mip < PREFILTER_MIP_LEVEL_COUNT; ++mip)
        {
            GLsizei mip_size = std::max<GLsizei>(1, PREFILTER_MAP_SIZE >> mip);
            for (int i = 0; i < 6; ++i)
            {
                std::string filename = (debug_dir / ("prefilter_mip" + std::to_string(mip) + "_" + 
                                                     std::string(face_names[i]) + ".png")).string();
                saveCubemapFaceToFile(prefilter_cubemap_, faces[i], mip_size, mip, filename);
            }
        }
        
        LOG_DEBUG("Saving BRDF LUT ({} px)", BRDF_LUT_SIZE);
        std::string brdf_filename = (debug_dir / "brdf_lut.png").string();
        saveBRDFLutToFile(brdf_lut_, BRDF_LUT_SIZE, brdf_filename);
        
        LOG_INFO("IBL debug textures saved to {}", debug_dir.string());
    }
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
        LOG_WARNING("Sky::bindIBL: IBL not ready, u_UseIBL=false");
        shader->setUniform("u_UseIBL", false);
        return;
    }

    shader->setUniform("u_UseIBL", true);

    const float max_reflection_lod = static_cast<float>(PREFILTER_MIP_LEVEL_COUNT - 1);
    shader->setUniform("u_MaxReflectionLod", max_reflection_lod);

    shader->bindTexture("u_IrradianceMap", 10);
    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_cubemap_);

    shader->bindTexture("u_PrefilterMap", 11);
    glActiveTexture(GL_TEXTURE11);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter_cubemap_);

    shader->bindTexture("u_BrdfLut", 12);
    glActiveTexture(GL_TEXTURE12);
    glBindTexture(GL_TEXTURE_2D, brdf_lut_);
}
