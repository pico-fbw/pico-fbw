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
    wifi.c
)

# Tell CMake to assemble the PIO file (for pwm.c; this generates pio.pwm.h)
pico_generate_pio_header(platform_pico ${CMAKE_CURRENT_LIST_DIR}/pwm.pio)

# Both USB and UART (pins moved) are enabled by default (either can be disabled by setting to 0, if needed)
# These are compiled into stdio, so using printf() will output to whatever is enabled
pico_enable_stdio_usb(platform_pico 1)
pico_enable_stdio_uart(platform_pico 1)
target_compile_definitions(platform_pico PRIVATE
    PICO_DEFAULT_UART_TX_PIN=12 # Must be a pin with UART0 TX
    PICO_DEFAULT_UART_RX_PIN=13 # Must be a pin with UART0 RX
)

# Link the platform library to the pico-sdk libraries
target_link_libraries(platform_pico
    pico_base
    pico_bootrom
    pico_platform
    pico_runtime # For libc
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
# Additionally, link to the cyw43 driver if the pico_w board is being used
if (FBW_PLATFORM STREQUAL "pico_w")
    # Link to the stdlib too, cyw43 driver uses it
    target_link_libraries(platform_pico pico_stdlib pico_cyw43_arch_lwip_threadsafe_background)
    # Include the platform directory so the lwipopts.h file can be found by the driver
    target_include_directories(platform_pico PUBLIC ${CMAKE_SOURCE_DIR}/platform/pico)
endif ()

# Include the source directory (so we can #include "platform/..." in files)
target_include_directories(platform_pico PRIVATE ${CMAKE_SOURCE_DIR})
