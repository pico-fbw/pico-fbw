add_library(fbw_io
    flash.c
    gps.c
    imu.c
    led.c
    pwm.c
    servo.c
    switch.c
)
pico_generate_pio_header(fbw_io ${CMAKE_CURRENT_LIST_DIR}/pwm.pio)

target_link_libraries(fbw_io
    pico_stdlib
    pico_binary_info
    pico_time
    hardware_flash
    hardware_gpio
    hardware_i2c
    hardware_pio
    hardware_pwm
    hardware_sync
    fbw_modes
)

if ("$CACHE{PICO_BOARD}" STREQUAL "pico_w")
    target_link_libraries(fbw_io pico_cyw43_arch_none)
    if (WIFLY_ENABLED)
        add_subdirectory(wifly)
    endif ()
endif ()
