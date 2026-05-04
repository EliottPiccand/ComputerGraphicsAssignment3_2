#include "Lib/OpenGL.h"

DepthMaskStateSaver::DepthMaskStateSaver(GLboolean new_state)
{
    glGetBooleanv(GL_DEPTH_WRITEMASK, &previous_state_);
    glDepthMask(new_state);
}

DepthMaskStateSaver::~DepthMaskStateSaver()
{
    glDepthMask(previous_state_);
}

CapabilityStateSaver::CapabilityStateSaver(GLenum capability, bool enable) : capability_(capability)
{
    was_enable_ = glIsEnabled(capability);
    if (enable)
        glEnable(capability);
    else
        glDisable(capability);
}

CapabilityStateSaver::~CapabilityStateSaver()
{
    if (was_enable_)
        glEnable(capability_);
    else
        glDisable(capability_);
}
