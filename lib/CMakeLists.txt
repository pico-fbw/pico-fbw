add_library(fbw_lib
    fusion/accel.c
    fusion/fusion.c
    fusion/gyro.c
    fusion/madgwick.c
    fusion/mag.c
    fusion/drivers/icm20948.c
    lfs.c
    lfs_util.c
    minmea.c
    nav.c
    parson.c
    pid.c
    semver.c
)

# fbw_sys is not included here
# Even though it does make use of sys/configuration and sys/print.h, this merely replaces code with macros and variable locations,
# and thus compilation is not necessary

target_include_directories(fbw_lib PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/src)
