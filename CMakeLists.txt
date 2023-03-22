cmake_minimum_required(VERSION 3.25)

# Import the Pico SDK
include (pico_sdk_import.cmake)

# Define project, language(s), and version
project(pico-fbw C CXX ASM)
set(CMAKE_C_STANDARD 11)

# Initialize the Pico SDK
pico_sdk_init()

# Point CMake to the source directory for building
add_subdirectory(src)

# Enable USB serial, disable UART serial (for debugging)
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)