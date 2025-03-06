target_sources(${PROJECT_NAME} PRIVATE
    fusion/fusion.c
    fusion/drivers/icm20948.c
    nav.c
    pid.c
)

if (SimConnect_FOUND)
    target_sources(${PROJECT_NAME} PRIVATE fusion/drivers/simconnect.c)
endif()
