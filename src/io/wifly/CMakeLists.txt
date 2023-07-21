set(wifly_sources wifly.c)

if ("$CACHE{PICO_BOARD}" STREQUAL "pico_w")
    set(wifly_sources ${wifly_sources}
        pcl/dhcp.c
        pcl/dns.c
        pcl/tcp.c
    )
endif ()

add_library(fbw_wifly ${wifly_sources})

target_link_libraries(fbw_wifly
    pico_stdlib
)
if ("$CACHE{PICO_BOARD}" STREQUAL "pico_w")
    target_link_libraries(fbw_wifly pico_cyw43_arch_lwip_threadsafe_background)
endif ()
