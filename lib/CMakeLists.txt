target_sources(${PROJECT_NAME} PRIVATE
    fusion/fusion.c
    fusion/drivers/bme280.c
    fusion/drivers/bmi270.c
    fusion/drivers/bmm350.c
    nav.c
    pid.c
)
