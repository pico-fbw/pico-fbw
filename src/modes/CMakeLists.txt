# Point CMake to C files for building
add_library(fbw_modes
    modes.c
    direct.c
    normal.c
)

# Import libraries
target_link_libraries(fbw_modes
    pico_stdlib
)