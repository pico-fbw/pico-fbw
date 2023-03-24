cmake_minimum_required(VERSION 3.13)

# Import the Pico SDK
include (pico_sdk_import.cmake)

# Define project, language(s), and version
project(pico-fbw C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# Point CMake to the source directory for building
add_subdirectory(src)

# Enable USB serial, disable UART serial (for debugging)
# TODO: Do NOT keep this in the final build! USB has a high overhead and I'm not exactly tethering my plane to a USB cable...
# I'll probably end up just disabling both because we don't need them.
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)