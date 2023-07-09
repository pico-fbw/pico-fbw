add_library(fbw_modes
    auto.c
    direct.c
    flight.c
    hold.c
    modes.c
    normal.c
    tune.c
)

target_link_libraries(fbw_modes
    pico_multicore
    pico_time
    fbw_io
    fbw_lib
)
if (WIFLY_ENABLED)
    target_link_libraries(fbw_modes fbw_wifly)
endif ()
