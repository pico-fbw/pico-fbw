#pragma once

// Include the correct pin definitions based on the platform (these macros are defined by CMake, in the root CMakeLists.txt)
#if defined(FBW_PLATFORM_ESP)
    #include "platform/esp/defs.h"
#elif defined(FBW_PLATFORM_HOST)
    #include "platform/host/defs.h"
#elif defined(FBW_PLATFORM_PICO)
    #include "platform/pico/defs.h"
#else
    #error "Something went wrong, check the current platform's .cmake files (FBW_PLATFORM_* not defined)"
#endif
