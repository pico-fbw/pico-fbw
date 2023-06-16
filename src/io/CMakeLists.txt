# Point CMake to C files for building
add_library(fbw_io
    flash.c
    gps.c
    imu.c
    led.c
    pwm.c
    servo.c
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
pico_generate_pio_header(fbw_io ${CMAKE_CURRENT_LIST_DIR}/pwm.pio)

# Import Pico W library
if ("$CACHE{PICO_BOARD}" STREQUAL "pico" OR "$CACHE{PICO_BOARD}" STREQUAL "pico_w")
    message(STATUS "[pico-fbw] $CACHE{PICO_BOARD} is compatable")
else ()
    message(WARNING "[pico-fbw] No compatable board found")
endif ()
if ("$CACHE{PICO_BOARD}" STREQUAL "pico_w")
    target_link_libraries(fbw_io pico_cyw43_arch_none)
    if (WIFLY_ENABLED)
        message(STATUS "[pico-fbw] Wi-Fly enabled")
        add_subdirectory(wifly)
    else ()
        message(STATUS "[pico-fbw] Wi-Fly disabled")
    endif ()
endif ()
