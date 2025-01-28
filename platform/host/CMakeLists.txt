add_library(platform_host
    flash.c
    gpio.c
    i2c.c
    pwm.c
    stdio.c
    sys.c
    time.c
    uart.c
)

if (FBW_BUILD_WWW)
    # We need to fetch mongoose as a dependency ("wifi" support requires it)
    include(${CMAKE_SOURCE_DIR}/platform/host/resources/mongoose.cmake)
    add_mongoose_to_target(platform_host)
    target_sources(platform_host PRIVATE wifi/wifi.c)
    # We need to include the generated/www directory so that the call to INCBIN in flash.c can find lfs.bin
    target_include_directories(platform_host PUBLIC ${CMAKE_BINARY_DIR}/generated/www)
endif()

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
    target_link_libraries(platform_host PRIVATE ws2_32) # For mongoose
endif()

target_include_directories(platform_host PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)
