# Point CMake to C files for building
add_library(fbw_io
    servo.c
)

# Import libraries
target_link_libraries(fbw_io
    pico_stdlib
    hardware_pwm
)