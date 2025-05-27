#pragma once

#include <stdbool.h>
#include "platform/types.h"

#include "sys/configuration.h"

#define ASDA (i16) config.pins[PINS_AAHRS_SDA]
#define ASCL (i16) config.pins[PINS_AAHRS_SCL]

typedef struct FusionDriver FusionDriver; // Forward declaration
typedef struct FusionDriver {
    byte addr;     // I2C address of the device
    void *context; // Arbitrary context that a driver may need
    /**
     * @param self pointer to the driver
     * @return true if this device is present and ready to be initialized
     * @note This function also sets the `addr` field of the driver if it succeeds.
     */
    bool (*exists)(FusionDriver *self);
    /**
     * Initializes the driver, including all needed internal configuration.
     * @param self pointer to the driver
     * @return true if the initialization was successful
     */
    bool (*init)(FusionDriver *self);
    /**
     * Should read data from the device and store it in the data array.
     * @param self pointer to the driver
     * @param data array to store the read data, formatted as a 3-element vector
     * (see the FusionDevice struct for details on this vector's format)
     * @return true if the read was successful
     */
    bool (*read)(FusionDriver *self, f32 data[3]);
    /**
     * Deinitializes the driver and frees any resources it may have allocated.
     * @param self pointer to the driver
     * @note This function is optional and may be NULL if not needed.
     */
    void (*destroy)(FusionDriver *self);
} FusionDriver;

typedef struct FusionDevice {
    // One or more drivers may be NULL
    FusionDriver *acc, *gyro, *mag;
    f32 accData[3];  // x, y, z (m/s^2)
    f32 gyroData[3]; // x, y, z (rad/s)
    f32 magData[3];  // x, y, z (uT)
    FusionDriver *baro;
    f32 baroData[3];  // pressure (Pa), temperature (°C), %RH (% 0-1)
    const char *name; // Human-readable identifier for the device
} FusionDevice;

/**
 * Helper function to check an I2C device's ID register (common on many devices).
 * @param driver pointer to the driver instance being checked
 * @param addr primary I2C address of the device
 * @param alt_addr alternate I2C address of the device (0 if not applicable)
 * @param reg ID register to read
 * @param expected expected value of the ID register
 * @return true if the device is present and has the expected ID
 * @note This function also sets the `addr` field of `driver` if it succeeds.
 */
bool check_devid(FusionDriver *driver, byte addr, byte alt_addr, byte reg, byte expected);

/**
 * Checks if a driver exists and attempts to initialize it if it does.
 * @param device pointer to the device
 * @param driver pointer to the driver to initialize
 * @param name human-readable identifier for the driver name
 * @return false if the driver does not exist or failed to initialize
 */
bool init_driver(const FusionDevice *device, FusionDriver *driver, const char *name);

/**
 * Deinitializes a driver, if applicable.
 * @param device pointer to the device
 * @param driver pointer to the driver to deinitialize
 * @param name human-readable identifier for the driver name
 */
void deinit_driver(const FusionDevice *device, FusionDriver *driver, const char *name);

extern const FusionDevice bme280;
extern const FusionDevice bmi270;
extern const FusionDevice bmm350;

extern const FusionDevice *fusionDevices[];
extern const u32 numFusionDevices;
