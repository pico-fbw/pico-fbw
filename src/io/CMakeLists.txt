add_library(fbw_io
    api.c
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
    pico_binary_info
    pico_platform
    pico_stdio
    pico_stdlib
    pico_time
    hardware_flash
    hardware_gpio
    hardware_i2c
    hardware_irq
    hardware_pio
    hardware_pwm
    hardware_sync
    hardware_uart
    fbw_lib
    fbw_modes
    fbw_wifly
)

if ("$CACHE{PICO_BOARD}" STREQUAL "pico_w")
    target_link_libraries(fbw_io pico_cyw43_arch_none)
endif ()

# Wi-Fly is always enabled in some capacity (to allow for the API to interact with it regardless of Pico W).
# LWIP and the whole HTTP stack only gets compiled if the Pico W is detected (this is all done in the Wi-Fly subdirectory).
add_subdirectory(wifly)
