add_library(platform_host
    flash.c
    gpio.c
    i2c.c
    pwm.c
    spi.c
    stdio.c
    sys.c
    time.c
    uart.c
    wifi.c
)

if (SimConnect_FOUND)
    message("Enabling build support for MSFS SimConnect")
    # We need to enable project C++ support to use SimConnect (a C++ API)
    project(${PROJECT_NAME} LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 17)
    target_sources(platform_host PRIVATE simconnect.cpp)
    target_include_directories(platform_host PRIVATE ${SimConnect_INCLUDE_DIRS})
    target_link_libraries(platform_host PRIVATE ${SimConnect_LIBRARIES})
endif()

if (CMAKE_HOST_WIN32)
    target_link_libraries(platform_host PRIVATE ws2_32 winmm) # For mongoose and time functions
endif()

target_include_directories(platform_host PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)
