include(DriverFirmware)

target_sources(${PROJECT_NAME} PRIVATE
    drivers/bmi323.c
    drivers/drivers.c
    fusion/fusion.c
    fusion/calibration.c
    fusion/calibration_motion.c
    fusion/calibration_still.c
    nav.c
    pid.c
)
