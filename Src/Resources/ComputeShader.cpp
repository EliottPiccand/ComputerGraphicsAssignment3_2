#include "Resources/ComputeShader.h"

#include <cassert>
#include <fstream>

#include "Resources/ResourceLoader.h"
#include "Utils/Log.h"
#include "Utils/Path.h"
#include "Utils/Profiling.h"

using namespace resource;

std::shared_ptr<ComputeShader> ComputeShader::load(const std::filesystem::path &partial_path,
                                                   const Defines defines)
{
    ProfileScope;

    constexpr const size_t MAX_SHADER_COMPILE_ERROR_REPORT_LENGTH = 2048;

    const auto path = ResourceLoader::ASSETS_DIRECTORY / DIRECTORY / partial_path;

    LOG_DEBUG("loading compute shader '{}'", relativeToExeDir(path).string());

    const auto defines_code = buildDefinesCode(defines);

    std::string compute_code;
    try
    {
        compute_code = buildShaderCode("compute", path, defines_code);
    }
    catch (const std::ifstream::failure &e)
    {
        LOG_ERROR("failed to load compute shader '{}': {}", relativeToExeDir(path).string(), e.what());
    }

    GLuint compute_shader = glCreateShader(GL_COMPUTE_SHADER);
    const char *compute_code_c_str = compute_code.c_str();
    glShaderSource(compute_shader, 1, &compute_code_c_str, nullptr);
    glCompileShader(compute_shader);

    GLint success;
    glGetShaderiv(compute_shader, GL_COMPILE_STATUS, &success);
    if (success != GL_TRUE)
    {
        char report[MAX_SHADER_COMPILE_ERROR_REPORT_LENGTH];
        glGetShaderInfoLog(compute_shader, MAX_SHADER_COMPILE_ERROR_REPORT_LENGTH, nullptr, report);
        LOG_ERROR("failed to compile compute shader '{}': {}", relativeToExeDir(path).string(), report);
        LOG_TRACE("compute shader code:\n{}", compute_code);

        throw std::runtime_error("compute shader compile errors");
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, compute_shader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char report[MAX_SHADER_COMPILE_ERROR_REPORT_LENGTH];
        glGetProgramInfoLog(program, MAX_SHADER_COMPILE_ERROR_REPORT_LENGTH, nullptr, report);
        LOG_ERROR("failed to link compute shader '{}': {}", relativeToExeDir(path).string(), report);
        throw std::runtime_error("compute shader linking error");
    }

    glDeleteShader(compute_shader);

    LOG_TRACE("loaded compute shader '{}' with id {}", relativeToExeDir(path).string(), program);

    return std::make_shared<ComputeShader>(program);
}
