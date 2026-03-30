add_library(platform_pico
    flash.c
    gpio.c
    i2c.c
    pwm.c
    stdio.c
    sys.c
    time.c
    uart.c
    wifi/dhcp.c
    wifi/dns.c
    wifi/tcp.c
    wifi/wifi.c
)

# Tell CMake to assemble the PIO file (for pwm.c; this generates pio.pwm.h)
pico_generate_pio_header(platform_pico ${CMAKE_CURRENT_LIST_DIR}/pwm.pio)

# Link the platform library to the pico-sdk libraries
target_link_libraries(platform_pico
    boot_picobin_headers
    hardware_clocks
    hardware_flash
    hardware_gpio
    hardware_i2c
    hardware_irq
    hardware_pio
    hardware_pwm
    hardware_regs
    hardware_sync
    hardware_timer
    hardware_uart
    hardware_watchdog
    pico_base_headers
    pico_binary_info
    pico_bootrom
    pico_clib_interface
    pico_platform
    pico_runtime
    pico_stdio
    pico_stdio_uart
    pico_stdio_usb
    pico_time
)

# Include the wifi directory so the lwipopts.h file can be found by lwIP
target_include_directories(platform_pico PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/wifi)
# Extra libraries for web interface support
if (FBW_PLATFORM STREQUAL "pico_w")
    # Link to stdlib (cyw43 driver uses it), and the cyw43 driver/lwIP
    target_link_libraries(platform_pico
        pico_stdlib
        pico_cyw43_arch_lwip_threadsafe_background
    )
else()
    # Link to lwIP and tinyusb for RNDIS support
    target_link_libraries(platform_pico
        pico_async_context_threadsafe_background
        pico_lwip_nosys
        pico_rand
        pico_unique_id
        tinyusb_device
    )
    # Include the tinyusb directory so tusb_config.h can be found by tinyusb
    target_include_directories(platform_pico PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tinyusb)
    # Add tinyusb's RNDIS support for USB networking
    if (NOT PICO_TINYUSB_PATH)
        set(PICO_TINYUSB_PATH ${PICO_SDK_PATH}/lib/tinyusb)
    endif()
    target_include_directories(platform_pico PUBLIC ${PICO_TINYUSB_PATH}/lib/networking)
    target_sources(platform_pico PUBLIC ${PICO_TINYUSB_PATH}/lib/networking/rndis_reports.c)
    # Add custom usb descriptors
    target_sources(platform_pico PUBLIC tinyusb/usb_descriptors.c)
    # USB reset via baudrate/vendor interface gets disabled when manually configuring tinyusb, so re-enable it
    target_compile_definitions(platform_pico PUBLIC
        PICO_STDIO_USB_ENABLE_RESET_VIA_BAUD_RATE=1
        PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE=1
    )
endif()


target_include_directories(platform_pico PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)
