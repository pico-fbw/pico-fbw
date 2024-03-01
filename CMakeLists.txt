cmake_minimum_required(VERSION 3.13)

# Establish the platform to build for, default is pico but the user can change this to any supported platform
if (NOT DEFINED FBW_PLATFORM)
    message("Defaulting FBW_PLATFORM to pico since not specified")
    message(STATUS "To specify a platform, run cmake with -DFBW_PLATFORM=<platform>")
endif()
set(FBW_PLATFORM "pico" CACHE STRING "Platform to build pico-fbw for")
# Special cases
if (FBW_PLATFORM STREQUAL "pico_w")
    set(PLATFORM_DIR "pico")
elseif (FBW_PLATFORM STREQUAL "esp32")
    set(PLATFORM_DIR "esp")
else()
    # Default to the platform name as its directory
    set(PLATFORM_DIR ${FBW_PLATFORM})
endif()
if (NOT EXISTS "${CMAKE_SOURCE_DIR}/platform/${PLATFORM_DIR}/cmake/${PLATFORM_DIR}.cmake")
    message(FATAL_ERROR "Unknown platform: ${FBW_PLATFORM}")
endif()
message("Building for platform: ${FBW_PLATFORM}")
message("Using platform directory: platform/${PLATFORM_DIR}")
include(platform/${PLATFORM_DIR}/cmake/${PLATFORM_DIR}.cmake)

# Define the project and the languages it uses
project(pico-fbw C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
# Enable all warnings + stop compilation on the first error for non-release builds (useful for project developers)
if (NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    message("Non-release build, all warnings enabled")
    add_compile_options(-Wall -Wextra -Wfatal-errors)
endif()

# Run pre-subdir setup (these setup functions are defined in the platform-specific CMake files that were included above)
message("Running prerequisite setup for ${FBW_PLATFORM}")
setup_before_subdirs()
# Add the rest of the directories (these have more CMake files that will be processed in turn)
message("Adding subdirectories")
add_subdirectory(src)
add_subdirectory(lib)
add_subdirectory(platform)
# Final setup tasks
message("Finishing up setup for ${FBW_PLATFORM}")
setup_after_subdirs()

