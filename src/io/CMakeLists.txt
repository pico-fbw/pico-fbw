# Point CMake to C files for building
add_library(fbw_io
    servo.c
    pwm.c
    imu.c
    led.c
    flash.c
)

# Import libraries
target_link_libraries(fbw_io
    pico_stdlib
    hardware_pwm
    hardware_i2c
    hardware_pio
    hardware_flash
    hardware_sync
)

# Import Pico W library
if ("$CACHE{PICO_BOARD}" STREQUAL "pico" OR "$CACHE{PICO_BOARD}" STREQUAL "pico_w")
    message(NOTICE "[pico-fbw] Board is compatable!")
    message(NOTICE "[pico-fbw] Building for target $CACHE{PICO_BOARD}")
else ()
    message(WARNING "[pico-fbw] No compatable board target found")
endif ()
if ("$CACHE{PICO_BOARD}" STREQUAL "pico_w")
    target_link_libraries(fbw_io pico_cyw43_arch_none)
    # Only compile in Wi-Fly if it is enabled (which is default) 
    option(WIFLY_ENABLED "Enable Wi-Fly support" ON)
    # check_symbol_exists(WIFLY "../config.h" WIFLY_ENABLED)
    if (WIFLY_ENABLED)
        message(STATUS "[pico-fbw] Wi-Fly build support enabled, use -DWIFLY_ENABLED=OFF to disable.")
        add_subdirectory(wifly)
    else ()
        message(STATUS "[pico-fbw] Wi-Fly build support disabled, use -DWIFLY_ENABLED=ON to enable.")
    endif ()
endif ()

# Auto generate the header file for the pio-asm
pico_generate_pio_header(fbw_io ${CMAKE_CURRENT_LIST_DIR}/pwm.pio)
