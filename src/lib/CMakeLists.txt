add_library(fbw_lib
    athr.c
    dmp.c
    minmea.c
    nav.c
    pid.c
)

target_link_libraries(fbw_lib
    pico_time
    hardware_i2c
)
