#pragma once

// Include the correct pin definitions based on the platform (these macros are defined by CMake, in the root CMakeLists.txt)
#ifdef FBW_PLATFORM_PICO
#include "platform/pico/defs.h"
#endif
