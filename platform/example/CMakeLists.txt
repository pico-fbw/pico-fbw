# Remember to change the name of the library to match your platform!

add_library(platform_example
    flash.c
    gpio.c
    i2c.c
    pwm.c
    stdio.c
    sys.c
    time.c
    uart.c
    # Feel free to add more files here as needed, for extra features or just if you prefer to split things up
)

target_link_libraries(platform_example
    # Define any libraries that are required for your platform here
    # (maybe SDK libraries, etc.)
)

target_include_directories(platform_example PRIVATE ${CMAKE_SOURCE_DIR})
