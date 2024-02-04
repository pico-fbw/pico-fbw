add_library(fbw_io
    aahrs.c
    display.c
    esc.c
    flash.c
    gps.c
    platform.c
    pwm.c
    serial.c
    servo.c
)

pico_generate_pio_header(fbw_io ${CMAKE_CURRENT_LIST_DIR}/pwm.pio)

target_link_libraries(fbw_io
    fbw_api
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
    hardware_structs
    hardware_sync
    hardware_uart
    hardware_watchdog
    pico_base
    pico_bootrom
    pico_platform
    pico_stdio
    pico_time
)

target_include_directories(fbw_io PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)
