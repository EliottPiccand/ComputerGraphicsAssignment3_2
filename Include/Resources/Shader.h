#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>

#include <Lib/OpenGL.h>
#include <Lib/glm.h>

namespace resource
{

class Shader
{
  public:
    static inline constexpr const std::string_view DIRECTORY = "Shaders";

    [[nodiscard]] static std::shared_ptr<Shader> loadFromFile(const std::filesystem::path &path);

    Shader(GLuint program);
    ~Shader();

    void bind() const;
    void setUniform(const char *name, bool value) const;
    void setUniform(const char *name, float value) const;
    void setUniform(const char *name, uint32_t value) const;
    void setUniform(const char *name, const glm::vec3 &value) const;
    void setUniform(const char *name, const glm::vec4 &value) const;
    void setUniform(const char *name, const glm::mat3 &value) const;
    void setUniform(const char *name, const glm::mat4 &value) const;

    void bindTexture(const char *name, GLint texture_unit) const;

  private:
    const GLuint program_;
};

} // namespace resource
