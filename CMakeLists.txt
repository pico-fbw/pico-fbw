cmake_minimum_required(VERSION 3.13)

# Establish the platform to build for, default is pico but the user can change this to any supported platform
set(FBW_PLATFORM "pico" CACHE STRING "Platform to build pico-fbw for")
if (FBW_PLATFORM STREQUAL "pico" OR FBW_PLATFORM STREQUAL "pico_w")
    add_definitions(-DFBW_PLATFORM_PICO)
    include(platform/pico/pico_sdk_import.cmake) # Using the pico-sdk
    set(PICO_BOARD ${FBW_PLATFORM}) # Forward the platform (normal Pico or W) to the pico-sdk
elseif (FBW_PLATFORM STREQUAL "esp32")
    add_definitions(-DFBW_PLATFORM_ESP32)
    include(platform/esp/esp_idf_import.cmake) # Using the ESP-IDF
else()
    message(FATAL_ERROR "Unknown platform: ${FBW_PLATFORM}")
endif()
message(STATUS "Building for platform: ${FBW_PLATFORM}")

# Define the project and the languages it uses
project(pico-fbw C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
# Enable all warnings + stop compilation on the first error for non-release builds (useful for project developers)
if (NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-Wall -Wextra -Wfatal-errors)
endif()

if (FBW_PLATFORM STREQUAL "pico" OR FBW_PLATFORM STREQUAL "pico_w")
    # The pico-sdk requires a very specific order of inclusion > initialization > add folders > configure things
    pico_sdk_init()
endif()

# Add the rest of the directories (these have more CMake files that will be processed in turn)
add_subdirectory(src)
add_subdirectory(lib)
add_subdirectory(platform)
