add_library(fbw_sys
    athr.c
    config.c
    info.c
    switch.c
)

target_link_libraries(fbw_sys
    fbw_io
    fbw_lib
    hardware_gpio
    pico_binary_info
)
