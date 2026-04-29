#include "Resources/ComputeShader.h"

#include <cassert>
#include <fstream>
#include <sstream>

#include "Utils/Log.h"
#include "Utils/Path.h"
#include "Utils/Profiling.h"

using namespace resource;

std::shared_ptr<ComputeShader> ComputeShader::loadFromFile(const std::filesystem::path &path_with_defines)
{
    ProfileScope;

    constexpr const size_t MAX_SHADER_COMPILE_ERROR_REPORT_LENGTH = 2048;

    LOG_DEBUG("loading compute shader '{}'", relativeToExeDir(path_with_defines).string());

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

    std::string compute_code;
    try
    {
        std::ifstream compute_shader_file;
        compute_shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        compute_shader_file.open(path);

        std::string compute_shader_version_line;

        std::getline(compute_shader_file, compute_shader_version_line);

        if (compute_shader_version_line.rfind("#version", 0) != 0)
        {
            LOG_ERROR("compute shader '{}' first line is not a #version directive, instead its '{}'",
                      path_with_defines.string(), compute_shader_version_line);
            throw std::runtime_error("compute shader first line is not a #version directive");
        }

        std::stringstream compute_shader_stream;

        compute_shader_stream << compute_shader_version_line << "\n" << defines << compute_shader_file.rdbuf();

        compute_shader_file.close();

        compute_code = compute_shader_stream.str();
    }
    catch (const std::ifstream::failure &e)
    {
        LOG_ERROR("failed to load shader '{}': {}", relativeToExeDir(path_with_defines).string(), e.what());
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
        LOG_ERROR("failed to compile shader '{}': {}", relativeToExeDir(path_with_defines).string(), report);
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
        LOG_ERROR("failed to link shader '{}': {}", relativeToExeDir(path_with_defines).string(), report);
        throw std::runtime_error("compute shader linking error");
    }

    glDeleteShader(compute_shader);

    return std::make_shared<ComputeShader>(program);
}
