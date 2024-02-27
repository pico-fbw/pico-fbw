add_subdirectory(api)

add_library(fbw_sys
    boot.c
    configuration.c
    log.c
    runtime.c
    throttle.c
    version.c
)

target_link_libraries(fbw_sys
    fbw_api
    fbw_io
    fbw_lib
    fbw_modes
    fbw_wifly
)

target_include_directories(fbw_sys PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)
