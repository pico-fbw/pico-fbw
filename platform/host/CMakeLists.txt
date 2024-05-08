add_library(platform_host
    flash.c
    gpio.c
    i2c.c
    pwm.c
    stdio.c
    sys.c
    time.c
    uart.c
)

target_include_directories(platform_host PRIVATE ${CMAKE_SOURCE_DIR})
