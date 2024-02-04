add_library(fbw_lib
    fusion/acc.c
    fusion/approx.c
    fusion/calibration.c
    fusion/fmath.c
    fusion/fusion.c
    fusion/mag.c
    fusion/matrix.c
    fusion/orient.c
    fusion/status.c
    fusion/drivers/drivers.c
    fusion/drivers/ak09916.c
    fusion/drivers/bno055.c
    fusion/drivers/icm20948.c
    minmea.c
    nav.c
    pid.c
    semver.c
)

target_link_libraries(fbw_lib
    fbw_io
    fbw_sys
    hardware_i2c
    hardware_gpio
    pico_base
    pico_time
)

target_include_directories(fbw_lib PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)
