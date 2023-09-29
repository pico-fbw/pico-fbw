add_library(fbw_api
    api.c
    cmds/cmds.c
)

target_link_libraries(fbw_api
    fbw_io
    fbw_lib
    fbw_modes
    fbw_wifly
    hardware_watchdog
    pico_base
    pico_platform
    pico_runtime
    pico_stdio
    pico_time
)
