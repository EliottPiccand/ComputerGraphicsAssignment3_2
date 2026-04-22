#include "Components/ModelInstance.h"

#include <cassert>

#include "Singleton.h"
#include "Utils/Constants.h"
#include "Utils/Profiling.h"

using namespace component;

ModelInstance::ModelInstance(std::shared_ptr<resource::Model> model, resource::Model::TextureOverride texture_override)
    : model_(model), texture_override_(texture_override)
{
}

bool ModelInstance::render() const
{
    ProfileScope;
    ProfileScopeGPU("ModelInstance::render");

    model_->draw(texture_override_);

    if (Singleton::debug)
    {
        constexpr const float LENGTH = 1.0f;

        constexpr const auto X_LINE_END = X * LENGTH;
        constexpr const auto Y_LINE_END = Y * LENGTH;
        constexpr const auto Z_LINE_END = Z * LENGTH;
        
        glLineWidth(2.0f);
        
        glBegin(GL_LINES);
            glMaterialfv(GL_FRONT, GL_AMBIENT, color::MATERIAL_RED);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, color::MATERIAL_RED);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(_v3(X_LINE_END));

            glMaterialfv(GL_FRONT, GL_AMBIENT, color::MATERIAL_GREEN);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, color::MATERIAL_GREEN);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(_v3(Y_LINE_END));            

            glMaterialfv(GL_FRONT, GL_AMBIENT, color::MATERIAL_BLUE);
            glMaterialfv(GL_FRONT, GL_DIFFUSE, color::MATERIAL_BLUE);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(_v3(Z_LINE_END));
        glEnd();
    }

    return false;
}
