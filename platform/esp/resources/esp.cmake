# See platform/example/resources/example.cmake for comments regarding the structure of this file
add_definitions(-DFBW_PLATFORM_ESP)

include(${CMAKE_CURRENT_LIST_DIR}/esp_idf_import.cmake) # Import ESP-IDF build system
# Misc CMake/compiler setup
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_TOOLCHAIN_FILE ${IDF_PATH}/tools/cmake/toolchain-${FBW_PLATFORM}.cmake)
set(CMAKE_EXECUTABLE_SUFFIX .elf)
# littlefs generation parameters
set(LFS_BLOCK_SIZE 4096)
set(LFS_PROG_SIZE 128)
set(LFS_IMG_SIZE 262144) # 256KB

# Expose flash size option to the user through CMake
set(ESP_FLASH_SIZE "4MB" CACHE STRING "Size of this ESP32's flash memory")
set(ESP_FLASH_SIZE_VALUES "2MB" "4MB" "8MB" "16MB")
if (NOT ESP_FLASH_SIZE IN_LIST ESP_FLASH_SIZE_VALUES)
    message(FATAL_ERROR "Invalid ESP_FLASH_SIZE: ${ESP_FLASH_SIZE}")
endif()

function(setup_before_subdirs)
    set(SDKCONFIG_DEFAULTS ${CMAKE_SOURCE_DIR}/platform/esp/resources/sdkconfig.defaults)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        # For debug builds we use a different set of defaults (namely just with more logging enabled)
        set(SDKCONFIG_DEFAULTS ${SDKCONFIG_DEFAULTS}.debug)
    endif()
    set(ESP_PARTTABLE_PATH ${CMAKE_SOURCE_DIR}/platform/esp/resources/partitions_no-ota.csv)
    message("Targeting ${ESP_FLASH_SIZE} flash esp models")
    # Calculate individual flash size options for sdkconfig
    set(ESP_FLASH_SIZE_2MB "n")
    set(ESP_FLASH_SIZE_4MB "n")
    set(ESP_FLASH_SIZE_8MB "n")
    set(ESP_FLASH_SIZE_16MB "n")
    if (ESP_FLASH_SIZE STREQUAL "2MB")
        set(ESP_FLASH_SIZE_2MB "y")
    elseif (ESP_FLASH_SIZE STREQUAL "4MB")
        set(ESP_FLASH_SIZE_4MB "y")
    elseif (ESP_FLASH_SIZE STREQUAL "8MB")
        set(ESP_FLASH_SIZE_8MB "y")
    elseif (ESP_FLASH_SIZE STREQUAL "16MB")
        set(ESP_FLASH_SIZE_16MB "y")
    endif()
    # Fill in the sdkconfig.defaults file
    configure_file(${SDKCONFIG_DEFAULTS}.in ${CMAKE_BINARY_DIR}/sdkconfig.defaults)
    # Invoke the ESP-IDF build process
    idf_build_process(${FBW_PLATFORM}
        PROJECT_VER
            ${PICO_FBW_VERSION}
        COMPONENTS
            # See IDF_PATH/components/README.md for a list of available components
            driver
            esptool_py
            esp_event
            esp_http_server
            esp_hw_support
            esp_netif
            esp_partition
            esp_system
            esp_timer
            esp_wifi
            freertos
            newlib
            nvs_flash
            soc
        SDKCONFIG
            ${CMAKE_BINARY_DIR}/sdkconfig
        SDKCONFIG_DEFAULTS
            ${CMAKE_BINARY_DIR}/sdkconfig.defaults
    )
    add_executable(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/src/main.c)
endfunction()

function(setup_after_subdirs)
    target_link_options(${PROJECT_NAME} PUBLIC "-Wl,--no-warn-rwx-segments") # Suppress warning about RWX segments
    idf_build_executable(${PROJECT_NAME})
endfunction()
