add_library(fbw_modes
    aircraft.c
    auto.c
    direct.c
    flight.c
    hold.c
    normal.c
    tune.c
)

target_link_libraries(fbw_modes
    fbw_io
    fbw_lib
    fbw_sys
    fbw_wifly
    pico_base
    pico_time
)
