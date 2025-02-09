# Use flash, stdio, sys, time, and wifi provided by the host platform; that functionality is exactly what we need
add_library(platform_linux
    ${CMAKE_SOURCE_DIR}/platform/host/flash.c
    gpio.c
    i2c.c
    pwm.c
    ${CMAKE_SOURCE_DIR}/platform/host/stdio.c
    ${CMAKE_SOURCE_DIR}/platform/host/sys.c
    ${CMAKE_SOURCE_DIR}/platform/host/time.c
    uart.c
    ${CMAKE_SOURCE_DIR}/platform/host/wifi.c
)

target_link_libraries(platform_linux
    gpiod
    m
    rt
)

target_include_directories(platform_linux PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)
