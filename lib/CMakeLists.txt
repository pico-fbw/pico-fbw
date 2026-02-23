include(DriverFirmware)

target_sources(${PROJECT_NAME} PRIVATE
    drivers/bme280.c
    drivers/bmi323.c
    drivers/drivers.c
    fusion.c
    nav.c
    pid.c
)

