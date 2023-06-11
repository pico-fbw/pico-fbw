cmake_minimum_required(VERSION 3.13)

# Import the Pico SDK
include(src/pico_sdk_import.cmake)

# Define project, languages and versions
project(pico-fbw C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# Point CMake to the source directory for building
add_subdirectory(src)

# If you are debugging the project, enable one of these options for a debug output!
# TODO: Disable USB in final build
pico_enable_stdio_usb(${PROJECT_NAME} 1)
# pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
# pico_enable_stdio_uart(${PROJECT_NAME} 1)
