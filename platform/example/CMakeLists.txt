add_library(platform_example
    flash.c
    gpio.c
    i2c.c
    pwm.c
    stdio.c
    sys.c
    time.c
    uart.c
)

target_link_libraries(platform_example
    
)

target_include_directories(platform_example PRIVATE ${CMAKE_SOURCE_DIR})
