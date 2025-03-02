target_sources(${PROJECT_NAME} PRIVATE
    fusion/fusion.c
    fusion/drivers/icm20948.c
    lfs.c
    lfs_util.c
    minmea.c
    nav.c
    parson.c
    pid.c
    semver.c
)

if (SimConnect_FOUND)
    target_sources(${PROJECT_NAME} PRIVATE fusion/drivers/simconnect.c)
endif()
