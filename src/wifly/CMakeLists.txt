set(wifly_sources wifly.c)

if ("$CACHE{PICO_BOARD}" STREQUAL "pico_w")
    set(wifly_sources ${wifly_sources}
        tcp_ip/dhcp.c
        tcp_ip/dns.c
        tcp_ip/tcp.c
    )
endif ()

add_library(fbw_wifly ${wifly_sources})

target_link_libraries(fbw_wifly
    fbw_io
    fbw_lib
    fbw_sys
    pico_base
    pico_platform
    pico_stdlib
)
if ("$CACHE{PICO_BOARD}" STREQUAL "pico_w")
    target_link_libraries(fbw_wifly pico_cyw43_arch_lwip_threadsafe_background)
endif ()

target_include_directories(fbw_wifly PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)
