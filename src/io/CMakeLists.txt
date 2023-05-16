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

# Import Pico W library
if ("$CACHE{PICO_BOARD}" STREQUAL "pico")
    message(NOTICE "[pico-fbw] Board is compatable!\n[pico-fbw] Compiling pico-fbw for standard Pico, please ensure this is the correct model") 
elseif ("$CACHE{PICO_BOARD}" STREQUAL "pico_w")
message(NOTICE "[pico-fbw] Board is compatable!\n[pico-fbw] Compiling pico-fbw for Pico W, please ensure this is the correct model") 
    target_link_libraries(fbw_io pico_cyw43_arch_none)
else()
    message(WARNING "[pico-fbw] No compatable Pico board target found")
endif ()

# Auto generate the header file for the pio-asm
pico_generate_pio_header(fbw_io
    ${CMAKE_CURRENT_LIST_DIR}/pwm.pio
)
