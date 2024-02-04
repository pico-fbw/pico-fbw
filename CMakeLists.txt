cmake_minimum_required(VERSION 3.13)

# Establish the platform to build for
set(PICO_FBW_PLATFORM "pico" CACHE STRING "Platform to build pico-fbw for")
if (PICO_FBW_PLATFORM STREQUAL "pico")
    # Using the Pico SDK
    include(pico_sdk_import.cmake)
    set(CMAKE_SYSTEM_PROCESSOR arm)
    set(CMAKE_OSX_ARCHITECTURES "") # Fix a build issue on M-series Macs
else()
    message(FATAL_ERROR "Unknown platform: ${PICO_FBW_PLATFORM}")
endif()
message(STATUS "Building for platform: ${PICO_FBW_PLATFORM}")

project(pico-fbw C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_FBW_PLATFORM STREQUAL "pico")
    # The Pico SDK requires a very specific order of inclusion > initialization > add folders > configure things
    pico_sdk_init()
    add_subdirectory(src)
    add_subdirectory(lib)
    add_subdirectory(platform/pico)
    # Both USB and UART (pins moved) are enabled by default (either can be disabled by setting to 0, if needed)
    pico_enable_stdio_usb(${PROJECT_NAME} 1)
    pico_enable_stdio_uart(${PROJECT_NAME} 1)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PICO_DEFAULT_UART_TX_PIN=12 # Must be a pin with UART0 TX
        PICO_DEFAULT_UART_RX_PIN=13 # Must be a pin with UART0 RX
    )
endif()
