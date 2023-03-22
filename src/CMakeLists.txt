# Point CMake to C files for building
add_executable(${PROJECT_NAME}
    main.c
)

# Option to build multiple files besides just UF2
pico_add_extra_outputs(${PROJECT_NAME})

# Import libraries
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
)