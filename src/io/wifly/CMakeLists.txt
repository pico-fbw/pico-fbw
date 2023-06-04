add_library(fbw_wifly
    wifly.c
    pcl/dhcp.c
    pcl/dns.c
    pcl/tcp.c
)

target_include_directories(fbw_wifly PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(fbw_wifly
    pico_cyw43_arch_lwip_threadsafe_background
    pico_stdlib
)
