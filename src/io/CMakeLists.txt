add_library(fbw_io
    api.c
    error.c
    esc.c
    flash.c
    gps.c
    imu.c
    platform.c
    pwm.c
    servo.c
)

pico_generate_pio_header(fbw_io ${CMAKE_CURRENT_LIST_DIR}/pwm.pio)

target_link_libraries(fbw_io
    fbw_lib
    fbw_modes
    fbw_sys
    fbw_wifly
    hardware_clocks
    hardware_flash
    hardware_gpio
    hardware_i2c
    hardware_irq
    hardware_pio
    hardware_pwm
    hardware_sync
    hardware_uart
    pico_base
    pico_platform
    pico_runtime
    pico_stdio
    pico_stdlib # IO is broken without this for some reason
    pico_time
)