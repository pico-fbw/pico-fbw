target_sources(${PROJECT_NAME} PRIVATE
    fusion/fusion.c
    fusion/drivers/bme280.c
    fusion/drivers/icm20948.c
    nav.c
    pid.c
)
