#include "Components/LightSource.h"

#include "Utils/Profiling.h"

using namespace component;

LightSource::LightSource(Color ambient, Color diffuse) : ambient_(ambient), diffuse_(diffuse)
{
    // if (available_lights_.empty())
    // {
    //     throw std::runtime_error("no more light source are available");
    // }

    // light_id_ = *available_lights_.begin();
    // available_lights_.erase(available_lights_.begin());

    // glEnable(light_id_);
}

LightSource::~LightSource()
{
    // glDisable(light_id_);
    // available_lights_.insert(light_id_);
}

void LightSource::render(glm::mat4 &transform) const
{
    ProfileScope;
    ProfileScopeGPU("LightSource::render");

    (void)transform;

    // constexpr const GLfloat light_position[] = {0.0f, 0.0f, 0.0f, 1.0f};
    // glLightfv(light_id_, GL_POSITION, light_position);

    // glLightfv(light_id_, GL_AMBIENT, reinterpret_cast<const GLfloat*>(&ambient_));
    // glLightfv(light_id_, GL_DIFFUSE, reinterpret_cast<const GLfloat*>(&diffuse_));

    // other settings :
    // GL_SPECULAR
    // GL_CONSTANT_ATTENUATION
    // GL_LINEAR_ATTENUATION
    // GL_QUADRATIC_ATTENUATION
    // GL_SPOT_DIRECTION
    // GL_SPOT_EXPONENT
    // GL_SPOT_CUTOFF
}
