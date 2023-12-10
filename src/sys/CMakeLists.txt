add_library(fbw_sys
    throttle.c
    config.c
    info.c
    log.c
    switch.c
)

target_link_libraries(fbw_sys
    fbw_io
    fbw_lib
    fbw_modes
    hardware_gpio
    hardware_watchdog
    pico_base
    pico_binary_info
    pico_time
)

if ("$CACHE{PICO_BOARD}" STREQUAL "pico_w")
    target_link_libraries(fbw_sys pico_cyw43_arch_none)
endif()

add_subdirectory(api)
