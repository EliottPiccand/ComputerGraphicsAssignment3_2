#pragma once

#include <memory>

#include <Lib/OpenGL.h>
#include <Lib/glm.h>

#include "Components/Component.h"
#include "Resources/Texture.h"

namespace component
{

/// Must be attached to the scene root only
class Sky : public Component
{
  public:
    Sky(std::weak_ptr<resource::Texture> texture);

    void initialize() override;
    void render(glm::mat4 &transform) const override;

    static void bindIBL(const std::shared_ptr<resource::Shader> &shader);

  private:
    static inline constexpr GLsizei ENVIRONMENT_MAP_SIZE = 512;
    static inline constexpr GLsizei IRRADIANCE_MAP_SIZE = 32;
    static inline constexpr GLsizei PREFILTER_MAP_SIZE = 128;
    static inline constexpr GLsizei BRDF_LUT_SIZE = 512;
    static inline constexpr int ENVIRONMENT_MIP_LEVEL_COUNT = 10;  // floor(log2(512)) + 1
    static inline constexpr int PREFILTER_MIP_LEVEL_COUNT = 5;

    static inline GLuint environment_cubemap_ = 0;
    static inline GLuint irradiance_cubemap_ = 0;
    static inline GLuint prefilter_cubemap_ = 0;
    static inline GLuint brdf_lut_ = 0;
    static inline bool ibl_ready_ = false;

    std::weak_ptr<resource::Texture> texture_;
};

} // namespace component
