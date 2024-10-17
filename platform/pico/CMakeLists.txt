# Define the platform library and its files
add_library(platform_pico
    adc.c
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
    pico_base_headers
    pico_bootrom
    pico_clib_interface
    pico_platform
    pico_runtime
    pico_stdio
    pico_stdio_uart
    pico_stdio_usb
    pico_time
    hardware_adc
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
)

# Extra steps for pico_w
if (FBW_PLATFORM STREQUAL "pico_w")
    # Include the wifi directory so the lwipopts.h file can be found by lwIP
    target_include_directories(platform_pico PUBLIC ${CMAKE_SOURCE_DIR}/platform/pico/wifi)
    # Link to stdlib (cyw43 driver uses it), and the cyw43 driver/lwIP
    target_link_libraries(platform_pico pico_stdlib pico_cyw43_arch_lwip_threadsafe_background)
endif()

# Include the root directory (so we can #include "platform/..." in files),
# and the source directory (so we can include "sys/...", etc. in files)
target_include_directories(platform_pico PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)
