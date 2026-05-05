#include <cstdlib>
#if !defined(OE_DEBUG)
#include <exception>
#endif

#pragma region libs

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <Lib/stb.h>

#define TINYGLTF3_IMPLEMENTATION
#include <Lib/tiny_gltf.h>

#pragma endregion libs

#include "Application.h"
#include "Utils/Log.h"

int main()
{
#if defined(OE_RELEASE)

#if defined(OE_DEBUG) || defined(OR_PROFILING)
#error "Only one of OE_RELEASE, OE_DEBUG and OR_PROFILING can be defined at the same time
#endif

#if defined(TRACY_ENABLE)
#error "TRACY_ENABLE must not be defined in Release mode"
#endif

    LOG_INFO("running in Release Mode");

#elif defined(OE_DEBUG)

#if defined(OE_RELEASE) || defined(OR_PROFILING)
#error "Only one of OE_RELEASE, OE_DEBUG and OR_PROFILING can be defined at the same time
#endif

#if defined(TRACY_ENABLE)
#error "TRACY_ENABLE must not be defined in Debug mode"
#endif

    LOG_INFO("running in Debug Mode");

#elif defined(OE_PROFILING)

#if defined(OE_RELEASE) || defined(OE_DEBUG)
#error "Only one of OE_RELEASE, OE_DEBUG and OR_PROFILING can be defined at the same time
#endif

#if !defined(TRACY_ENABLE)
#error "TRACY_ENABLE must be defined in Profiling mode"
#endif

    LOG_INFO("running in Profiling Mode");

#endif

#if !defined(OE_DEBUG)
    try
    {
#endif
        Application app;
        app.run();
#if !defined(OE_DEBUG)
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("an exception occured: {}", e.what());
        return EXIT_FAILURE;
    }
#endif

    return EXIT_SUCCESS;
}
