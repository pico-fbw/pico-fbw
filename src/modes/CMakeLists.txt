add_library(fbw_modes
    aircraft.c
    auto.c
    direct.c
    flight.c
    hold.c
    normal.c
    tune.c
)

target_link_libraries(fbw_modes
    fbw_io
    fbw_lib
    fbw_sys
)

target_include_directories(fbw_modes PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)
