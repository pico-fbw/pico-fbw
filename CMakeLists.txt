cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_OSX_ARCHITECTURES "") # M-series Mac build support

project(pico-fbw C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_subdirectory(src)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 1)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    PICO_DEFAULT_UART_TX_PIN=12 # Must be a pin with UART0 RX (no, this is not a typo, it's just weird design)
    PICO_DEFAULT_UART_RX_PIN=13 # Must be a pin with UART0 TX
)
