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
# Import Pico W specific libraries and set compiler definitions
if ("$CACHE{PICO_BOARD}" STREQUAL "pico_w")
    target_link_libraries(${PROJECT_NAME} pico_cyw43_arch_none)
    if (WIFLY_ENABLED)
        target_link_libraries(${PROJECT_NAME} fbw_wifly)
        add_compile_definitions(WIFLY_ENABLED)
        include_directories(${CMAKE_CURRENT_LIST_DIR}/io/wifly)
    endif ()
endif ()

# Point CMake to other project directories
add_subdirectory(io)
add_subdirectory(lib)
add_subdirectory(modes)


# Reset program:

# Define the pico-fbw-reset executable target
add_executable(pico-fbw-reset
    reset.c
)
# Import libraries
target_link_libraries(pico-fbw-reset
    pico_bootrom
    fbw_io
)
# Set compile options for the pico-fbw-reset executable
target_compile_options(pico-fbw-reset
    PRIVATE
    -Wall
    -Wextra
)
# Set the executable to upload to RAM because we are dealing with erasing flash
pico_set_binary_type(pico-fbw-reset no_flash)
# Option to build multiple files
pico_add_extra_outputs(pico-fbw-reset)
# Install the pico-fbw-reset executable in the same directory as the main executable
install(TARGETS pico-fbw-reset DESTINATION ${CMAKE_INSTALL_PREFIX})

add_custom_command(TARGET pico-fbw-reset POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rename "${CMAKE_BINARY_DIR}/src/pico-fbw-reset.uf2" "${CMAKE_BINARY_DIR}/pico-fbw-reset.uf2"
)
