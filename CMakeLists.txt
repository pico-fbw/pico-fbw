cmake_minimum_required(VERSION 3.13)

include(src/pico_sdk_import.cmake)
# For M-series Mac compatability
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_OSX_ARCHITECTURES "")

project(pico-fbw C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_subdirectory(src)

# IMPORTANT: If you are debugging the project, enable either of these options for a debug output!
# The system will automatically output a complete debug log to either of these.
# Default baudrate is 115200 and USB is enabled by default
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)
