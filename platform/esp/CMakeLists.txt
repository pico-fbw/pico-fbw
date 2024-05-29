# See platform/pico/CMakeLists.txt for comments regarding the structure of this file
add_library(platform_esp
    flash.c
    gpio.c
    i2c.c
    pwm.c
    stdio.c
    sys.c
    time.c
    uart.c
    wifi/dns.c
    wifi/http.c
    wifi/wifi.c
)

# Link the platform library to the pico-sdk libraries
target_link_libraries(platform_esp
    idf::driver
    idf::esp_event
    idf::esp_http_server
    idf::esp_hw_support
    idf::esp_netif
    idf::esp_partition
    idf::esp_system
    idf::esp_timer
    idf::esp_wifi
    idf::freertos
    idf::newlib
    idf::nvs_flash
    idf::soc
)

target_include_directories(platform_esp PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)
