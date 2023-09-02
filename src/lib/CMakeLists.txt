add_library(fbw_lib
    athr.c
    dmp.c
    info.c
    minmea.c
    nav.c
    pid.c
)

target_link_libraries(fbw_lib
    fbw_io
    hardware_gpio
    hardware_i2c
    pico_base
    pico_binary_info
)
