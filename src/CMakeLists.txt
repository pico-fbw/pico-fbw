add_executable(${PROJECT_NAME}
    main.c
)

pico_add_extra_outputs(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    fbw_io
    fbw_lib
    fbw_modes
)
# Import Pico W specific libraries and set specific compiler definitions, also a few messages about board compatability
if ("$CACHE{PICO_BOARD}" STREQUAL "pico" OR "$CACHE{PICO_BOARD}" STREQUAL "pico_w")
    message(STATUS "[pico-fbw] $CACHE{PICO_BOARD} is compatable")
else ()
    message(WARNING "[pico-fbw] No compatable board found")
    set(WIFLY_ENABLED OFF)
endif ()
if ("$CACHE{PICO_BOARD}" STREQUAL "pico_w")
    target_link_libraries(${PROJECT_NAME} pico_cyw43_arch_none)
    option(WIFLY_ENABLED "Enable Wi-Fly (pico-fbw)" ON) # create this option if we haven't already
    if (WIFLY_ENABLED) # check_symbol_exists(WIFLY "config.h" WIFLY_ENABLED)
        message(STATUS "[pico-fbw] Wi-Fly enabled")
        add_compile_definitions(WIFLY_ENABLED)
        include_directories(${CMAKE_CURRENT_LIST_DIR}/io/wifly)
    else ()
        message(STATUS "[pico-fbw] Wi-Fly disabled")
    endif ()
endif ()

add_subdirectory(io)
add_subdirectory(lib)
add_subdirectory(modes)


# Reset program:

add_executable(pico-fbw-reset
    reset.c
)
target_link_libraries(pico-fbw-reset
    pico_bootrom
    fbw_io
)
target_compile_options(pico-fbw-reset
    PRIVATE
    -Wall
    -Wextra
)
pico_set_binary_type(pico-fbw-reset no_flash) # set the executable to upload to RAM because we are dealing with erasing flash
pico_add_extra_outputs(pico-fbw-reset)
install(TARGETS pico-fbw-reset DESTINATION ${CMAKE_INSTALL_PREFIX})

# At the end of the build process, move the final binaries to the buildroot for easier access
add_custom_command(TARGET pico-fbw POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rename "${CMAKE_BINARY_DIR}/src/pico-fbw.uf2" "${CMAKE_BINARY_DIR}/pico-fbw.uf2"
)
add_custom_command(TARGET pico-fbw-reset POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rename "${CMAKE_BINARY_DIR}/src/pico-fbw-reset.uf2" "${CMAKE_BINARY_DIR}/pico-fbw-reset.uf2"
)
