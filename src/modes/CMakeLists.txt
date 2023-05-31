# Point CMake to C files for building
add_library(fbw_modes
    modes.c
    direct.c
    normal.c
    auto.c
    tune.c
)

# Import libraries
target_link_libraries(fbw_modes
    pico_stdlib
    pico_multicore
    fbw_lib
    fbw_io
)
if (WIFLY_ENABLED)
    target_link_libraries(fbw_modes fbw_wifly)
endif ()
