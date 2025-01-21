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

if (NOT HOST_SKIP_WWW)
    # We need to fetch mongoose as a dependency (wifi emulation requires it)
    include(FetchContent)
    FetchContent_Declare(
        mongoose
        GIT_REPOSITORY https://github.com/cesanta/mongoose
        GIT_TAG 7.16
    )
    FetchContent_MakeAvailable(mongoose)
    target_sources(platform_host PRIVATE ${mongoose_SOURCE_DIR}/mongoose.c wifi/wifi.c)
    target_include_directories(platform_host PRIVATE ${mongoose_SOURCE_DIR})
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
