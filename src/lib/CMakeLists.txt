add_library(fbw_lib
    minmea.c
    nav.c
    pid.c
    semver.c
)

target_link_libraries(fbw_lib
    fbw_io
    fbw_sys
    hardware_i2c
    pico_base
)
