#include "Resources/Texture.h"

#include <stdexcept>

#include <Lib/stb.h>

#include "Utils/Log.h"
#include "Utils/Path.h"
#include "Utils/Profiling.h"

using namespace resource;

Texture::Texture(GLuint id) : id_(id)
{
}

Texture::~Texture()
{
    if (id_ != 0)
    {
        glDeleteTextures(1, &id_);
    }
}

std::shared_ptr<Texture> Texture::loadFromFile(const std::filesystem::path &path)
{
    ProfileScope;

    LOG_DEBUG("loading texture '{}'", relativeToExeDir(path).string());

    int width, height, channels;
    unsigned char *data = stbi_load(path.generic_string().c_str(), &width, &height, &channels, 0);

    if (data == nullptr)
    {
        LOG_ERROR("failed to load texture '{}'", relativeToExeDir(path).string());
        throw std::runtime_error("failed to load texture");
    }

    GLenum format;
    switch (channels)
    {
    case 1:
        format = GL_RED;
        break;
    case 3:
        format = GL_RGB;
        break;
    case 4:
        format = GL_RGBA;
        break;
    default:
        LOG_ERROR("failed to load texture from memory: invalid channel count {}", channels);
        throw std::runtime_error("texture loading failed");
    }

    GLuint id;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format), width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(data);

    return std::make_shared<Texture>(id);
}

void Texture::bind(GLenum slot, std::shared_ptr<resource::Shader> shader, const char *uniform_slot) const
{
    glActiveTexture(slot);
    glBindTexture(GL_TEXTURE_2D, id_);
    shader->bindTexture(uniform_slot, static_cast<GLint>(slot - GL_TEXTURE0));
}
