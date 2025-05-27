include(DriverFirmware)

target_sources(${PROJECT_NAME} PRIVATE
    fusion/fusion.c
    fusion/drivers/bme280.c
    fusion/drivers/bmi270.c
    fusion/drivers/bmm350.c
    fusion/drivers/drivers.c
    nav.c
    pid.c
)

# Generate headers from binary firmware blobs to be used by fusion drivers
generate_firmware_headers(
    bmi270.bin
)
