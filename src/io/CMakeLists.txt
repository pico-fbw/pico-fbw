add_library(fbw_io
    aahrs.c
    display.c
    esc.c
    gps.c
    receiver.c
    servo.c
)

target_link_libraries(fbw_io
    fbw_lib
    fbw_modes
    fbw_sys
    fbw_wifly
)

target_include_directories(fbw_io PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)
