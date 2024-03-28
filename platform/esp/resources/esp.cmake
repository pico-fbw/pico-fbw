# See platform/pico/resources/pico.cmake for comments regarding the structure of this file
add_definitions(-DFBW_PLATFORM_ESP)

include(${CMAKE_CURRENT_LIST_DIR}/esp_idf_import.cmake)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_TOOLCHAIN_FILE ${IDF_PATH}/tools/cmake/toolchain-${FBW_PLATFORM}.cmake)
set(CMAKE_EXECUTABLE_SUFFIX .elf)
set(LFS_BLOCK_SIZE 4096)
set(LFS_PROG_SIZE 128)
set(LFS_IMG_SIZE 262144) # 256KB

function(setup_before_subdirs)
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        set(SDKCONFIG_DEFAULTS_PATH ${CMAKE_SOURCE_DIR}/platform/esp/resources/sdkconfig.defaults)
    else()
        # For non-release builds we use a different set of defaults (namely just with more logging enabled)
        set(SDKCONFIG_DEFAULTS_PATH ${CMAKE_SOURCE_DIR}/platform/esp/resources/sdkconfig.defaults.debug)
    endif()
    idf_build_process(${FBW_PLATFORM}
        PROJECT_VER
            ${PICO_FBW_VERSION}
        COMPONENTS
            # See IDF_PATH/components/README.md for a list of available components
            driver
            esptool_py
            esp_hw_support
            esp_partition
            esp_system
            esp_timer
            freertos
            newlib
            nvs_flash
            soc
        SDKCONFIG
            ${CMAKE_SOURCE_DIR}/platform/esp/resources/sdkconfig
        SDKCONFIG_DEFAULTS
            ${SDKCONFIG_DEFAULTS_PATH}
    )
    add_executable(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/src/main.c)
endfunction()

function(setup_after_subdirs)
    target_link_options(${PROJECT_NAME} PUBLIC "-Wl,--no-warn-rwx-segments") # Suppress warning about RWX segments
    idf_build_executable(${PROJECT_NAME})
endfunction()
