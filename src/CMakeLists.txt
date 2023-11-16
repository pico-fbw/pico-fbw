add_executable(${PROJECT_NAME} main.c)
pico_add_extra_outputs(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
    fbw_api
    fbw_io
    fbw_lib
    fbw_modes
    fbw_sys
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
    message(STATUS "[pico-fbw] Board checks passed")
else ()
    message(WARNING "[pico-fbw] No compatable board found")
endif ()
if ("$CACHE{PICO_BOARD}" STREQUAL "pico_w")
    target_link_libraries(${PROJECT_NAME} pico_cyw43_arch_none)
    include_directories(${CMAKE_CURRENT_LIST_DIR}/wifly)
endif ()

add_subdirectory(io)
add_subdirectory(lib)
add_subdirectory(modes)
add_subdirectory(sys)
# Wi-Fly is always enabled in some capacity (to allow for the API to interact with it regardless of Pico W).
# LWIP and the whole HTTP stack only gets compiled if the Pico W is detected (this is all done in the Wi-Fly subdirectory).
add_subdirectory(wifly)

# At the end of the build process, move the final binaries to the buildroot for easier access
add_custom_command(TARGET pico-fbw POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rename "${CMAKE_BINARY_DIR}/src/${PROJECT_NAME}.uf2" "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.uf2"
)
