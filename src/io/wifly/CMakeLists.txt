add_library(fbw_wifly
    wifly.c
    dhcp/dhcp.c
    dns/dns.c
)

target_include_directories(fbw_wifly PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/dhcp
    ${CMAKE_CURRENT_LIST_DIR}/dns
)

target_link_libraries(fbw_wifly
    pico_cyw43_arch_lwip_threadsafe_background
    pico_stdlib
)
