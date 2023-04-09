# Point CMake to C files for building
add_executable(${PROJECT_NAME}
    main.c
)

# Option to build multiple files
pico_add_extra_outputs(${PROJECT_NAME})

# Import libraries
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    fbw_io
    fbw_lib
    fbw_modes
)

# Point CMake to other project directories
add_subdirectory(io)
add_subdirectory(lib)
add_subdirectory(modes)


# Add executable, target, etc for reset program

# Define the pico-fbw-reset executable target
add_executable(pico-fbw-reset
    reset.c
)
# Import libraries
target_link_libraries(pico-fbw-reset
    pico_stdlib
    fbw_io
)
# Set compile options for the pico-fbw-reset executable
target_compile_options(pico-fbw-reset
    PRIVATE
    -Wall
    -Wextra
)
# Set the executable to upload to RAM just in case, because we are dealing with erasing flash
pico_set_binary_type(pico-fbw-reset no_flash)
# Option to build multiple files
pico_add_extra_outputs(pico-fbw-reset)
# Set include directories for the pico-fbw-reset executable
target_include_directories(pico-fbw-reset
    PRIVATE
    include
)
# Install the pico-fbw-reset executable in the same directory as the main executable
install(TARGETS pico-fbw-reset DESTINATION ${CMAKE_INSTALL_PREFIX})