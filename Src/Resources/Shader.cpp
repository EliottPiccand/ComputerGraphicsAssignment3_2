#include "Resources/Shader.h"

#include <cassert>
#include <format>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>

#include "Resources/ResourceLoader.h"
#include "Utils/Log.h"
#include "Utils/Path.h"
#include "Utils/Profiling.h"

using namespace resource;

constexpr const size_t MAX_SHADER_COMPILE_ERROR_REPORT_LENGTH = 2048;

Shader::Shader(GLuint program) : program_(program)
{
}

Shader::~Shader()
{
    glDeleteProgram(program_);
}

std::shared_ptr<Shader> Shader::load(
    const std::filesystem::path &partial_vertex_path, const std::filesystem::path &partial_fragment_path,
    const Defines defines, const std::vector<std::filesystem::path> &shared_code_paths,
    const std::optional<std::reference_wrapper<const std::filesystem::path>> &partial_tesselation_control_path,
    const std::optional<std::reference_wrapper<const std::filesystem::path>> &partial_tesselation_evaluation_path)
{
    ProfileScope;

    const auto vertex_path = ResourceLoader::ASSETS_DIRECTORY / DIRECTORY / partial_vertex_path;
    const auto fragment_path = ResourceLoader::ASSETS_DIRECTORY / DIRECTORY / partial_fragment_path;
    const auto tcs_path = partial_tesselation_control_path.transform(
        [](auto p) { return ResourceLoader::ASSETS_DIRECTORY / DIRECTORY / p; });
    const auto tes_path = partial_tesselation_evaluation_path.transform(
        [](auto p) { return ResourceLoader::ASSETS_DIRECTORY / DIRECTORY / p; });

    LOG_DEBUG("loading shaders:");
    LOG_DEBUG("- vertex:   '{}'", relativeToExeDir(vertex_path).string());
    LOG_DEBUG("- fragment: '{}'", relativeToExeDir(fragment_path).string());
    LOG_DEBUG("- TCS:      '{}'",
              tcs_path.transform([](auto p) { return relativeToExeDir(p).string(); }).value_or("none"));
    LOG_DEBUG("- TES:      '{}'",
              tes_path.transform([](auto p) { return relativeToExeDir(p).string(); }).value_or("none"));

    const auto defines_code = buildDefinesCode(defines);
    const auto [vertex_shared_code, fragment_shared_code, tcs_shared_code, tes_shared_code] =
        buildSharedCode(shared_code_paths);

    std::string vertex_code;
    std::string fragment_code;
    std::optional<std::string> tcs_code = std::nullopt;
    std::optional<std::string> tes_code = std::nullopt;
    try
    {
        vertex_code = buildShaderCode("vertex", vertex_path, defines_code, vertex_shared_code);
        fragment_code = buildShaderCode("fragment", fragment_path, defines_code, fragment_shared_code);
        if (tcs_path.has_value())
        {
            tcs_code = buildShaderCode("TCS", tcs_path.value(), defines_code, tcs_shared_code);
        }
        if (tes_path.has_value())
        {
            tes_code = buildShaderCode("TCS", tes_path.value(), defines_code, tes_shared_code);
        }
    }
    catch (const std::ifstream::failure &e)
    {
        LOG_ERROR("failed to load shader:");
        LOG_ERROR("- vertex:   '{}'", relativeToExeDir(vertex_path).string());
        LOG_ERROR("- fragment: '{}'", relativeToExeDir(fragment_path).string());
        LOG_ERROR("- TCS:      '{}'",
                  tcs_path.transform([](auto p) { return relativeToExeDir(p).string(); }).value_or("none"));
        LOG_ERROR("- TES:      '{}'",
                  tes_path.transform([](auto p) { return relativeToExeDir(p).string(); }).value_or("none"));
        LOG_ERROR("=> {}", e.what());
        throw std::runtime_error("shader loading fails");
    }

    bool compile_error = false;

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    if (compileShader(vertex_shader, vertex_code))
    {
        LOG_ERROR("failed to compile vertex shader '{}'", relativeToExeDir(vertex_path).string());
        compile_error = true;
    }

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    if (compileShader(fragment_shader, fragment_code))
    {
        LOG_ERROR("failed to compile fragment shader '{}'", relativeToExeDir(fragment_path).string());
        compile_error = true;
    }

    std::optional<GLuint> tcs_shader =
        tcs_code.has_value() ? std::optional(glCreateShader(GL_TESS_CONTROL_SHADER)) : std::nullopt;
    if (tcs_shader.has_value() && compileShader(tcs_shader.value(), tcs_code.value()))
    {
        LOG_ERROR("failed to compile TCS shader '{}'", relativeToExeDir(tcs_path.value()).string());
        compile_error = true;
    }

    std::optional<GLuint> tes_shader =
        tes_code.has_value() ? std::optional(glCreateShader(GL_TESS_EVALUATION_SHADER)) : std::nullopt;
    if (tes_shader.has_value() && compileShader(tes_shader.value(), tes_code.value()))
    {
        LOG_ERROR("failed to compile TES shader '{}'", relativeToExeDir(tes_path.value()).string());
        compile_error = true;
    }

    if (compile_error)
    {
        throw std::runtime_error("Shader compile errors");
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    if (tcs_shader.has_value())
        glAttachShader(program, tcs_shader.value());
    if (tes_shader.has_value())
        glAttachShader(program, tes_shader.value());
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char report[MAX_SHADER_COMPILE_ERROR_REPORT_LENGTH];
        glGetProgramInfoLog(program, MAX_SHADER_COMPILE_ERROR_REPORT_LENGTH, nullptr, report);

        LOG_ERROR("failed to link shader:");
        LOG_ERROR("- vertex:   '{}'", relativeToExeDir(vertex_path).string());
        LOG_ERROR("- fragment: '{}'", relativeToExeDir(fragment_path).string());
        LOG_ERROR("- TCS:      '{}'",
                  tcs_path.transform([](auto p) { return relativeToExeDir(p).string(); }).value_or("none"));
        LOG_ERROR("- TES:      '{}'",
                  tes_path.transform([](auto p) { return relativeToExeDir(p).string(); }).value_or("none"));
        LOG_ERROR("=> {}", report);
        throw std::runtime_error("Shader linking error");
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    LOG_TRACE("loaded shader:");
    LOG_TRACE("- vertex:   '{}'", relativeToExeDir(vertex_path).string());
    LOG_TRACE("- fragment: '{}'", relativeToExeDir(fragment_path).string());
    LOG_TRACE("- TCS:      '{}'",
              tcs_path.transform([](auto p) { return relativeToExeDir(p).string(); }).value_or("none"));
    LOG_TRACE("- TES:      '{}'",
              tes_path.transform([](auto p) { return relativeToExeDir(p).string(); }).value_or("none"));
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

void Shader::setUniform(const char *name, const glm::vec2 &value) const
{
    const auto uniform_location = glGetUniformLocation(program_, name);
    glUniform2fv(uniform_location, 1, glm::value_ptr(value));
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

GLint Shader::getUniformLocation(const char *name) const
{
    return glGetUniformLocation(program_, name);
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
                                    const std::string &defines, const std::string &shared_code)
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
    shader_stream << shader_version_line << "\n" << defines << "\n" << shared_code << shader_file.rdbuf();

    shader_file.close();

    return shader_stream.str();
}

std::tuple<std::string, std::string, std::string, std::string> Shader::buildSharedCode(
    const std::vector<std::filesystem::path> &shared_code_paths)
{
    std::stringstream vertex_code_steam;
    std::stringstream fragment_code_steam;
    std::stringstream tcs_code_steam;
    std::stringstream tes_code_steam;
    for (const auto &path : shared_code_paths)
    {
        std::ifstream file;
        file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        file.open(ResourceLoader::ASSETS_DIRECTORY / DIRECTORY / path);

        const auto str = path.string();

        std::stringstream *sstream;
        if (str.ends_with(".vert"))
        {
            sstream = &vertex_code_steam;
        }
        else if (str.ends_with(".frag"))
        {
            sstream = &fragment_code_steam;
        }
        else if (str.ends_with("TCS.glsl"))
        {
            sstream = &tcs_code_steam;
        }
        else if (str.ends_with("TES.glsl"))
        {
            sstream = &tes_code_steam;
        }
        else
        {
            LOG_ERROR("shader '{}' is not recognised as a vertex, fragment, TES or TCS shader", str);
            throw std::runtime_error("unknown shader extension");
        }

        (*sstream) << file.rdbuf() << "\n";
    }
    return {vertex_code_steam.str(), fragment_code_steam.str(), tcs_code_steam.str(), tes_code_steam.str()};
}

bool Shader::compileShader(GLuint shader, std::string code)
{
    const char *code_c_str = code.c_str();
    glShaderSource(shader, 1, &code_c_str, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE)
    {
        char report[MAX_SHADER_COMPILE_ERROR_REPORT_LENGTH];
        glGetShaderInfoLog(shader, MAX_SHADER_COMPILE_ERROR_REPORT_LENGTH, nullptr, report);
        LOG_ERROR("shader compile error: {}", report);
        LOG_TRACE("vertex shader code:\n{}", code);
        return true;
    }

    return false;
}
