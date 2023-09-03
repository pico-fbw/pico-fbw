add_executable(${PROJECT_NAME} main.c)
pico_add_extra_outputs(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
    fbw_io
    fbw_lib
    fbw_modes
    fbw_wifly
    hardware_watchdog
    pico_base
    pico_bootrom
    pico_platform
    pico_stdio
    pico_time
)

# Import Pico W specific libraries and set specific compiler definitions
if ("$CACHE{PICO_BOARD}" STREQUAL "pico" OR "$CACHE{PICO_BOARD}" STREQUAL "pico_w")
    message(STATUS "[pico-fbw] $CACHE{PICO_BOARD} is compatable")
else ()
    message(WARNING "[pico-fbw] No compatable board found")
endif ()
if ("$CACHE{PICO_BOARD}" STREQUAL "pico_w")
    target_link_libraries(${PROJECT_NAME} pico_cyw43_arch_none)
    # WIFLY_ENABLED now just basically serves as a stand-in for RASPBERRYPI_PICO_W; it cannot be manually enabled/disabled
    # It's still used in a few places though, and the behavior could possibly be altered...
    add_compile_definitions(WIFLY_ENABLED)
    include_directories(${CMAKE_CURRENT_LIST_DIR}/wifly)
endif ()

add_subdirectory(io)
add_subdirectory(lib)
add_subdirectory(modes)

# Wi-Fly is always enabled in some capacity (to allow for the API to interact with it regardless of Pico W).
# LWIP and the whole HTTP stack only gets compiled if the Pico W is detected (this is all done in the Wi-Fly subdirectory).
add_subdirectory(wifly)


# Reset program:
add_executable(pico-fbw-reset reset.c)
pico_add_extra_outputs(pico-fbw-reset)
target_link_libraries(pico-fbw-reset
    fbw_io
    pico_bootrom
)

pico_set_binary_type(pico-fbw-reset no_flash) # set the executable to upload to RAM because we are dealing with erasing flash

install(TARGETS pico-fbw-reset DESTINATION ${CMAKE_INSTALL_PREFIX})

# At the end of the build process, move the final binaries to the buildroot for easier access
add_custom_command(TARGET pico-fbw POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rename "${CMAKE_BINARY_DIR}/src/${PROJECT_NAME}.uf2" "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.uf2"
)
add_custom_command(TARGET pico-fbw-reset POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rename "${CMAKE_BINARY_DIR}/src/pico-fbw-reset.uf2" "${CMAKE_BINARY_DIR}/pico-fbw-reset.uf2"
)
