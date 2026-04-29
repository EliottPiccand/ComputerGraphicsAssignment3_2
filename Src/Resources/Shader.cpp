#include "Resources/Shader.h"

#include <cassert>
#include <fstream>
#include <sstream>

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

std::shared_ptr<Shader> Shader::loadFromFile(const std::filesystem::path &path_with_defines)
{
    ProfileScope;

    constexpr const size_t MAX_SHADER_COMPILE_ERROR_REPORT_LENGTH = 2048;

    LOG_DEBUG("loading shaders '{}.vert/.frag'", relativeToExeDir(path_with_defines).string());

    const auto splitPathAndDefines = [](const std::string &str) {
        std::vector<std::string> tokens;
        std::string token;
        std::stringstream ss(str);
        while (std::getline(ss, token, '#'))
            tokens.push_back(token);

        std::string name = tokens.front();
        std::vector<std::string> params(tokens.begin() + 1, tokens.end());

        return std::make_tuple(name, params);
    };

    const auto [path, defines_list] = splitPathAndDefines(path_with_defines.string());

    std::stringstream defines_stream;
    for (const auto &define : defines_list)
    {
        defines_stream << "#define " << define << "\n";
    }
    const auto defines = defines_stream.str();

    std::string vertex_code;
    std::string fragment_code;
    try
    {
        std::ifstream vertex_shader_file;
        std::ifstream fragment_shader_file;
        vertex_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fragment_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        vertex_shader_file.open(path + ".vert");
        fragment_shader_file.open(path + ".frag");

        std::string vertex_shader_version_line;
        std::string fragment_shader_version_line;

        std::getline(vertex_shader_file, vertex_shader_version_line);
        std::getline(fragment_shader_file, fragment_shader_version_line);

        if (vertex_shader_version_line.rfind("#version", 0) != 0)
        {
            LOG_ERROR("vertex shader '{}' first line is not a #version directive, instead its '{}'",
                      path_with_defines.string(), vertex_shader_version_line);
            throw std::runtime_error("vertex shader first line is not a #version directive");
        }
        if (fragment_shader_version_line.rfind("#version", 0) != 0)
        {
            LOG_ERROR("fragment shader '{}' first line is not a #version directive, instead its '{}'",
                      path_with_defines.string(), fragment_shader_version_line);
            throw std::runtime_error("fragment shader first line is not a #version directive");
        }

        std::stringstream vertex_shader_stream;
        std::stringstream fragment_shader_stream;

        vertex_shader_stream << vertex_shader_version_line << "\n" << defines << vertex_shader_file.rdbuf();
        fragment_shader_stream << fragment_shader_version_line << "\n" << defines << fragment_shader_file.rdbuf();

        vertex_shader_file.close();
        fragment_shader_file.close();

        vertex_code = vertex_shader_stream.str();
        fragment_code = fragment_shader_stream.str();
    }
    catch (const std::ifstream::failure &e)
    {
        LOG_ERROR("failed to load shader '{}': {}", relativeToExeDir(path_with_defines).string(), e.what());
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
        LOG_ERROR("failed to compile shader '{}.vert': {}", relativeToExeDir(path_with_defines).string(), report);
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
        LOG_ERROR("failed to compile shader '{}.frag': {}", relativeToExeDir(path_with_defines).string(), report);
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
        LOG_ERROR("failed to link shader '{}': {}", relativeToExeDir(path_with_defines).string(), report);
        throw std::runtime_error("Shader linking error");
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

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

void Shader::bindTexture(const char *name, GLint texture_unit) const
{
    const auto uniform_location = glGetUniformLocation(program_, name);
    glUniform1i(uniform_location, texture_unit);
}
