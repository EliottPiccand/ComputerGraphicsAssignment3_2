#include "Components/Text.h"

#include <Lib/glm.h>

#include "Utils/Color.h"
#include "Utils/Constants.h"

using namespace component;

Text::Text(float width, float height, std::shared_ptr<resource::Texture> texture)
    : width_(width), height_(height), texture_(texture)
{
}

bool Text::render() const
{
    const auto half_width = width_ / 2.0f;
    const auto half_height = height_ / 2.0f;

    const auto top_right    = half_width * EAST + half_height * NORTH;
    const auto top_left     = half_width * WEST + half_height * NORTH;
    const auto bottom_left  = half_width * WEST + half_height * SOUTH;
    const auto bottom_right = half_width * EAST + half_height * SOUTH;

    const auto ambient = glm::vec4(glm::vec3(color::WHITE) * 0.5f, color::WHITE.w);
    GLfloat ambient_color[] = {_v4(ambient)};
    GLfloat diffuse_color[] = {_v4(color::WHITE)};
    GLfloat specular_color[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient_color);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse_color);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular_color);

    glEnable(GL_TEXTURE_2D);
    texture_->bind(GL_TEXTURE0);

    glBegin(GL_QUADS);
        glTexCoord2f(1.0f, 0.0f); glVertex3f(_v3(top_right));
        glTexCoord2f(0.0f, 0.0f); glVertex3f(_v3(top_left));
        glTexCoord2f(0.0f, 1.0f); glVertex3f(_v3(bottom_left));
        glTexCoord2f(1.0f, 1.0f); glVertex3f(_v3(bottom_right));
    glEnd();

    texture_->unbind(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);

    return false;
}
