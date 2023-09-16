add_library(fbw_lib
    dmp.c
    minmea.c
    nav.c
    pid.c
    semver.c
)

target_link_libraries(fbw_lib
    fbw_io
    hardware_i2c
    pico_base
)
