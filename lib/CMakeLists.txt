include(DriverFirmware)

target_sources(${PROJECT_NAME} PRIVATE
    drivers/bme280.c
    drivers/bmi270.c
    drivers/bmm350.c
    drivers/drivers.c
    nav.c
    pid.c
)

# Generate headers from binary firmware blobs to be used by fusion drivers
generate_firmware_headers(
    bmi270.bin
)
