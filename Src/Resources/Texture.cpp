#include "Resources/Texture.h"

#include <cassert>
#include <stdexcept>

#include <Lib/stb.h>

#include "Resources/ResourceLoader.h"
#include "Utils/Log.h"
#include "Utils/Path.h"
#include "Utils/Profiling.h"

using namespace resource;

Texture::Texture(GLuint id, Type type) : type_(type), id_(id)
{
}

Texture::~Texture()
{
    if (id_ != 0)
    {
        glDeleteTextures(1, &id_);
    }
}

std::shared_ptr<Texture> Texture::load(const std::filesystem::path &partial_path, Type type, bool is_hdr)
{
    ProfileScope;

    const auto path = ResourceLoader::ASSETS_DIRECTORY / DIRECTORY / partial_path;

    LOG_DEBUG("loading texture '{}'", relativeToExeDir(path).string());

    int width, height, channels;
    void *data;
    if (is_hdr)
    {
        data = stbi_loadf(path.generic_string().c_str(), &width, &height, &channels, 0);
    }
    else
    {
        data = stbi_load(path.generic_string().c_str(), &width, &height, &channels, 0);
    }

    if (data == nullptr)
    {
        LOG_ERROR("failed to load texture '{}'", relativeToExeDir(path).string());
        throw std::runtime_error("failed to load texture");
    }

    const bool use_srgb = type == Type::Albedo || type == Type::Emissive;

    GLenum raw_format;
    GLint internal_format;
    if (is_hdr)
    {
        switch (channels)
        {
        case 1:
            raw_format = GL_RED;
            internal_format = GL_R16F;
            break;
        case 3:
            raw_format = GL_RGB;
            internal_format = GL_RGB16F;
            break;
        case 4:
            raw_format = GL_RGBA;
            internal_format = GL_RGBA16F;
            break;
        default:
            LOG_ERROR("failed to load HDR texture from memory: invalid channel count {}", channels);
            throw std::runtime_error("texture loading failed");
        }
    }
    else
    {
        switch (channels)
        {
        case 1:
            raw_format = GL_RED;
            internal_format = GL_RED;
            break;
        case 3:
            raw_format = GL_RGB;
            internal_format = use_srgb ? GL_SRGB8 : GL_RGB8;
            break;
        case 4:
            raw_format = GL_RGBA;
            internal_format = use_srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
            break;
        default:
            LOG_ERROR("failed to load texture from memory: invalid channel count {}", channels);
            throw std::runtime_error("texture loading failed");
        }
    }

    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    GLenum underlying_type = is_hdr ? GL_FLOAT : GL_UNSIGNED_BYTE;
    glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, raw_format, underlying_type, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    if (is_hdr)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    else
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);

    LOG_TRACE("loaded texture '{}' with id {}", relativeToExeDir(path).string(), id);

    return std::make_shared<Texture>(id, type);
}

void Texture::bind(GLenum slot, std::shared_ptr<resource::Shader> shader, const char *uniform_slot) const
{
    if (shader->bindTexture(uniform_slot, static_cast<GLint>(slot - GL_TEXTURE0)))
    {
        glActiveTexture(slot);
        glBindTexture(GL_TEXTURE_2D, id_);
    }
}

Texture::Type Texture::getType() const
{
    return type_;
}
