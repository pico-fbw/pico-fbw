# Point CMake to C files for building
add_library(fbw_io
    servo.c
    pwm.c
    imu.c
    led.c
    flash.c
)

# Import libraries
target_link_libraries(fbw_io
    pico_stdlib
    hardware_pwm
    hardware_i2c
    hardware_pio
    hardware_flash
    hardware_sync
)

# Auto generate the header file for the pio-asm
pico_generate_pio_header(fbw_io
    ${CMAKE_CURRENT_LIST_DIR}/pwm.pio
)
