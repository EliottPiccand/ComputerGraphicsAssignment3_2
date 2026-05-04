#include "Resources/Shader.h"

#include <cassert>
#include <format>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "Resources/ResourceLoader.h"
#include "Utils/Log.h"
#include "Utils/Path.h"
#include "Utils/Profiling.h"

using namespace resource;

Shader::Shader(GLuint program) : program_(program)
{
}

Shader::~Shader()
{
    glDeleteProgram(program_);
}

std::shared_ptr<Shader> Shader::load(const std::filesystem::path &partial_vertex_path,
                                     const std::filesystem::path &partial_fragment_path, const Defines defines)
{
    ProfileScope;

    constexpr const size_t MAX_SHADER_COMPILE_ERROR_REPORT_LENGTH = 2048;

    const auto vertex_path = ResourceLoader::ASSETS_DIRECTORY / DIRECTORY / partial_vertex_path;
    const auto fragment_path = ResourceLoader::ASSETS_DIRECTORY / DIRECTORY / partial_fragment_path;

    LOG_DEBUG("loading shaders:");
    LOG_DEBUG("- vertex:   '{}'", relativeToExeDir(vertex_path).string());
    LOG_DEBUG("- fragment: '{}'", relativeToExeDir(fragment_path).string());

    const auto defines_code = buildDefinesCode(defines);

    std::string vertex_code;
    std::string fragment_code;
    try
    {
        vertex_code = buildShaderCode("vertex", vertex_path, defines_code);
        fragment_code = buildShaderCode("fragment", fragment_path, defines_code);
    }
    catch (const std::ifstream::failure &e)
    {
        LOG_ERROR("failed to load shader:");
        LOG_ERROR("- vertex:   '{}'", relativeToExeDir(vertex_path).string());
        LOG_ERROR("- fragment: '{}'", relativeToExeDir(fragment_path).string());
        LOG_ERROR("=> {}", e.what());
        throw std::runtime_error("shader loading fails");
    }

    bool compile_error = false;

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    const char *vertex_code_c_str = vertex_code.c_str();
    glShaderSource(vertex_shader, 1, &vertex_code_c_str, nullptr);
    glCompileShader(vertex_shader);

    GLint success;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE)
    {
        char report[MAX_SHADER_COMPILE_ERROR_REPORT_LENGTH];
        glGetShaderInfoLog(vertex_shader, MAX_SHADER_COMPILE_ERROR_REPORT_LENGTH, nullptr, report);
        LOG_ERROR("failed to compile vertex shader '{}': {}", relativeToExeDir(vertex_path).string(), report);
        LOG_TRACE("vertex shader code:\n{}", vertex_code);
        compile_error = true;
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    const char *fragment_code_c_str = fragment_code.c_str();
    glShaderSource(fragment_shader, 1, &fragment_code_c_str, nullptr);
    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE)
    {
        char report[MAX_SHADER_COMPILE_ERROR_REPORT_LENGTH];
        glGetShaderInfoLog(fragment_shader, MAX_SHADER_COMPILE_ERROR_REPORT_LENGTH, nullptr, report);
        LOG_ERROR("failed to compile fragment shader '{}': {}", relativeToExeDir(fragment_path).string(), report);
        LOG_TRACE("fragment shader code:\n{}", fragment_code);
        compile_error = true;
    }

    if (compile_error)
    {
        throw std::runtime_error("Shader compile errors");
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char report[MAX_SHADER_COMPILE_ERROR_REPORT_LENGTH];
        glGetProgramInfoLog(program, MAX_SHADER_COMPILE_ERROR_REPORT_LENGTH, nullptr, report);

        LOG_ERROR("failed to link shader:");
        LOG_ERROR("- vertex:   '{}'", relativeToExeDir(vertex_path).string());
        LOG_ERROR("- fragment: '{}'", relativeToExeDir(fragment_path).string());
        LOG_ERROR("=> {}", report);
        throw std::runtime_error("Shader linking error");
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    LOG_TRACE("loaded shader:");
    LOG_TRACE("- vertex:   '{}'", relativeToExeDir(vertex_path).string());
    LOG_TRACE("- fragment: '{}'", relativeToExeDir(fragment_path).string());
    LOG_TRACE("=> {}", program);

    return std::make_shared<Shader>(program);
}

void Shader::bind() const
{
    glUseProgram(program_);
}

void Shader::setUniform(const char *name, bool value) const
{
    const auto uniform_location = glGetUniformLocation(program_, name);
    glUniform1i(uniform_location, value ? 1 : 0);
}

void Shader::setUniform(const char *name, int32_t value) const
{
    const auto uniform_location = glGetUniformLocation(program_, name);
    glUniform1i(uniform_location, value);
}

void Shader::setUniform(const char *name, uint32_t value) const
{
    const auto uniform_location = glGetUniformLocation(program_, name);
    glUniform1ui(uniform_location, value);
}

void Shader::setUniform(const char *name, float value) const
{
    const auto uniform_location = glGetUniformLocation(program_, name);
    glUniform1f(uniform_location, value);
}

void Shader::setUniform(const char *name, const glm::vec3 &value) const
{
    const auto uniform_location = glGetUniformLocation(program_, name);
    // LOG_TRACE("u loc {}", uniform_location);
    glUniform3fv(uniform_location, 1, glm::value_ptr(value));
}

void Shader::setUniform(const char *name, const glm::vec4 &value) const
{
    const auto uniform_location = glGetUniformLocation(program_, name);
    glUniform4fv(uniform_location, 1, glm::value_ptr(value));
}

void Shader::setUniform(const char *name, const glm::mat3 &value) const
{
    const auto uniform_location = glGetUniformLocation(program_, name);
    glUniformMatrix3fv(uniform_location, 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setUniform(const char *name, const glm::mat4 &value) const
{
    const auto uniform_location = glGetUniformLocation(program_, name);
    glUniformMatrix4fv(uniform_location, 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setUniformArrayElement(std::string_view name, size_t index, const glm::vec3 &value) const
{
    const auto uniform_name = std::vformat(name, std::make_format_args(index));
    const auto uniform_location = glGetUniformLocation(program_, uniform_name.c_str());
    glUniform3fv(uniform_location, 1, glm::value_ptr(value));
}

bool Shader::bindTexture(const char *name, GLint texture_unit) const
{
    const auto uniform_location = glGetUniformLocation(program_, name);
    glUniform1i(uniform_location, texture_unit);
    return uniform_location != -1;
}

std::string Shader::buildDefinesCode(const Defines defines)
{
    std::stringstream defines_stream;
    for (const auto &[name, value_option] : defines)
    {
        defines_stream << "#define " << name;

        if (value_option.has_value())
            defines_stream << " " << value_option.value();

        defines_stream << "\n";
    }
    return defines_stream.str();
}

std::string Shader::buildShaderCode(std::string_view type, const std::filesystem::path &path,
                                    const std::string &defines)
{
    std::ifstream shader_file;
    shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    shader_file.open(path);

    std::string shader_version_line;
    std::getline(shader_file, shader_version_line);

    if (shader_version_line.rfind("#version", 0) != 0)
    {
        LOG_ERROR("{} shader '{}' first line is not a #version directive, instead its '{}'", type, path.string(),
                  shader_version_line);
        throw std::runtime_error("shader first line is not a #version directive");
    }

    std::stringstream shader_stream;
    shader_stream << shader_version_line << "\n" << defines << shader_file.rdbuf();

    shader_file.close();

    return shader_stream.str();
}
