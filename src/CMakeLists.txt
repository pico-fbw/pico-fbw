# Point CMake to C files for building
add_executable(${PROJECT_NAME}
    main.c
)

# Option to build multiple files besides just UF2
pico_add_extra_outputs(${PROJECT_NAME})

# Import libraries
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_pwm
    fbw_io
    fbw_lib
    fbw_modes
)

# Point CMake to other project directories
add_subdirectory(io)
add_subdirectory(lib)
add_subdirectory(modes)