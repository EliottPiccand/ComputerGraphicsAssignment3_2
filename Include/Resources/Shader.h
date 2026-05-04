#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <Lib/OpenGL.h>
#include <Lib/glm.h>

namespace resource
{

class Shader
{
  public:
    using Defines = std::unordered_map<std::string_view, std::optional<std::string>>;

    static inline constexpr const std::string_view DIRECTORY = "Shaders";

    [[nodiscard]] static std::shared_ptr<Shader> load(const std::filesystem::path &vertex_path,
                                                      const std::filesystem::path &fragment_path,
                                                      const Defines defines = {});

    Shader(GLuint program);
    ~Shader();

    void bind() const;
    void setUniform(const char *name, bool value) const;
    void setUniform(const char *name, float value) const;
    void setUniform(const char *name, int32_t value) const;
    void setUniform(const char *name, uint32_t value) const;
    void setUniform(const char *name, const glm::vec3 &value) const;
    void setUniform(const char *name, const glm::vec4 &value) const;
    void setUniform(const char *name, const glm::mat3 &value) const;
    void setUniform(const char *name, const glm::mat4 &value) const;
    
    void setUniformArrayElement(std::string_view name, size_t index, const glm::vec3 &value) const;

    bool bindTexture(const char *name, GLint texture_unit) const;

  protected:
    static std::string buildDefinesCode(const Defines defines);
    static std::string buildShaderCode(std::string_view type, const std::filesystem::path &path,
                                       const std::string &defines);

  private:
    const GLuint program_;
};

} // namespace resource
