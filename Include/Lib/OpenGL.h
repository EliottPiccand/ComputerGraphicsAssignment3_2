#pragma once

#pragma clang diagnostic push

#pragma clang diagnostic ignored "-Wreserved-identifier"
#pragma clang diagnostic ignored "-Wnonportable-system-include-path"
#pragma clang diagnostic ignored "-Wreserved-macro-identifier"
#pragma clang diagnostic ignored "-Wlanguage-extension-token"

#include <GL/glew.h>
#include <GL/glu.h>

#define _DELETE_MOVE_AND_COPY(Class)                                                                                   \
    Class(const Class &) = delete;                                                                                     \
    Class &operator=(const Class &) = delete;                                                                          \
    Class(Class &&) = delete;                                                                                          \
    Class &operator=(Class &&) = delete

class DepthMaskStateSaver
{
  public:
    DepthMaskStateSaver(GLboolean new_state);
    ~DepthMaskStateSaver();

    _DELETE_MOVE_AND_COPY(DepthMaskStateSaver);

  private:
    GLboolean previous_state_;
};


class CapabilityStateSaver
{
  public:
    CapabilityStateSaver(GLenum capability, bool enable);
    ~CapabilityStateSaver();

    _DELETE_MOVE_AND_COPY(CapabilityStateSaver);

  private:
    GLenum capability_;
    GLboolean was_enable_;
};

#define _RAII_3(line, type, ...) [[maybe_unused]] type opengl_state_scope_##line(__VA_ARGS__)
#define _RAII_2(line, type, ...) _RAII_3(line, type __VA_OPT__(,) __VA_ARGS__) 
#define _RAII_1(type, ...) _RAII_2(__LINE__, type __VA_OPT__(,) __VA_ARGS__)

#define SCOPE_DEPTH_MASK(state) _RAII_1(DepthMaskStateSaver, state)
#define SCOPE_DISABLE(cap) _RAII_1(CapabilityStateSaver, cap, false)

#pragma clang diagnostic pop
