# Point CMake to C files for building
add_library(fbw_io
    servo.c
    pwm.c
)

# Import libraries
target_link_libraries(fbw_io
    pico_stdlib
    hardware_pwm
    hardware_pio
)

# Auto generate the header file for the pio-asm
pico_generate_pio_header(fbw_io
    ${CMAKE_CURRENT_LIST_DIR}/pwm.pio
)