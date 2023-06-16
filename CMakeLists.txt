cmake_minimum_required(VERSION 3.13)

project(pico-fbw C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include(src/pico_sdk_import.cmake)
pico_sdk_init()

add_subdirectory(src)

# If you are debugging the project, enable one of these options for a debug output!
# TODO: Disable USB in final build
pico_enable_stdio_usb(${PROJECT_NAME} 1) # 0
pico_enable_stdio_uart(${PROJECT_NAME} 0) # 1
