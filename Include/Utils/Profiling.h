#pragma once

#if defined(TRACY_ENABLE)

#include <Lib/OpenGL.h>
#include <tracy/Tracy.hpp>
#include <tracy/TracyOpenGL.hpp>

#define ProfilingEndFrame FrameMark
#define ProfileScope ZoneScoped 0
#define SetGpuProfilingContext TracyGpuContext 0
#define CollectGpuProfilingEvents TracyGpuCollect 0
#define ProfileScopeGPU(name) TracyGpuZone(name) 0

#else

#define ProfilingEndFrame
#define ProfileScope
#define SetGpuProfilingContext
#define CollectGpuProfilingEvents
#define ProfileScopeGPU(name)

#endif
