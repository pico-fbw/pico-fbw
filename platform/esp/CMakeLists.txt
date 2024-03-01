# See platform/pico/CMakeLists.txt for comments regarding the structure of this file
add_library(platform_esp
    flash.c
    gpio.c
    i2c.c
    pwm.c
    stdio.c
    sys.c
    time.c
    uart.c
)

# Link the platform library to the pico-sdk libraries
target_link_libraries(platform_esp
    idf::driver
)

target_include_directories(platform_esp PRIVATE ${CMAKE_SOURCE_DIR})
