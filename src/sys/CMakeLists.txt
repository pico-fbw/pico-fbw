add_library(fbw_sys
    athr.c
    config.c
    info.c
    switch.c
)

target_link_libraries(fbw_sys
    fbw_io
    fbw_lib
    fbw_modes
    hardware_gpio
    pico_base
    pico_binary_info
)

add_subdirectory(api)
