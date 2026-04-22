#pragma once

#pragma clang diagnostic push

#pragma clang diagnostic ignored "-Wreserved-identifier"
#pragma clang diagnostic ignored "-Wnonportable-system-include-path"
#pragma clang diagnostic ignored "-Wreserved-macro-identifier"
#pragma clang diagnostic ignored "-Wlanguage-extension-token"

#include <GL/glew.h>
#include <GL/glu.h>

#define _v2(vector) vector.x, vector.y
#define _v3(vector) vector.x, vector.y, vector.z
#define _v4(vector) vector.x, vector.y, vector.z, vector.w
#define _dv3(vector) static_cast<double>(vector.x), static_cast<double>(vector.y), static_cast<double>(vector.z)

#define PUSH_CLEAR_STATE()                                                                                             \
    glMatrixMode(GL_MODELVIEW);                                                                                        \
    glPushMatrix();                                                                                                    \
    glMatrixMode(GL_PROJECTION);                                                                                       \
    glPushMatrix()

#define POP_CLEAR_STATE()                                                                                              \
    glMatrixMode(GL_PROJECTION);                                                                                       \
    glPopMatrix();                                                                                                     \
    glMatrixMode(GL_MODELVIEW);                                                                                        \
    glPopMatrix()

#pragma clang diagnostic pop
