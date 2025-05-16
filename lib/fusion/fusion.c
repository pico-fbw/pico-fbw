/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "platform/helpers.h"

#include "sys/print.h"

#include "drivers/drivers.h"

#include "fusion.h"

#define MAX_DEVICES 5 // The maximum number of devices that are supported at once

// Master device list; devices are defined in their respective driver source files
static const FusionDevice *devices[] = {
    &bme280,
    &bmi270,
    &bmm350,
};
static const FusionDevice *detected[MAX_DEVICES];
static u32 detectedCount = 0;

/**
 * Checks if a driver exists and attempts to initialize it if it does.
 * @param device pointer to the device
 * @param driver pointer to the driver to initialize
 * @param name human-readable identifier for the driver name
 * @return false if the driver does not exist or failed to initialize
 */
static bool init_driver(const FusionDevice *device, FusionDriver *driver, const char *name) {
    if (!driver || !driver->exists(driver)) {
        return false;
    }
    if (!driver->init(driver)) {
        printsys(aahrs, "WARNING: detected %s for '%s' but failed to initialize", name, device->name);
        return false;
    }
    printsys(aahrs, "initialized %s for '%s'", name, device->name);
    return true;
}

/**
 * Deinitializes a driver, if applicable.
 * @param device pointer to the device
 * @param driver pointer to the driver to deinitialize
 * @param name human-readable identifier for the driver name
 */
static void deinit_driver(const FusionDevice *device, FusionDriver *driver, const char *name) {
    if (driver && driver->destroy) {
        driver->destroy(driver);
        printsys(aahrs, "deinitialized %s for '%s'", name, device->name);
    }
}

bool fusion_init() {
    // Scan through all known devices and attempt to init their drivers
    printsys(aahrs, "detecting fusion devices");
    bool detectedAcc = false, detectedGyro = false;
    for (u32 i = 0; i < count_of(devices); i++) {
        const FusionDevice *dev = devices[i];
        bool acc = init_driver(dev, dev->acc, "accelerometer");
        bool gyro = init_driver(dev, dev->gyro, "gyroscope");
        bool mag = init_driver(dev, dev->mag, "magnetometer");
        bool baro = init_driver(dev, dev->baro, "barometer");
        if (acc || gyro || mag || baro) {
            printsys(aahrs, "successfully detected and initialized '%s'", dev->name);
            detected[detectedCount++] = dev;
            detectedAcc |= acc;
            detectedGyro |= gyro;
        }
    }
    // Fusion needs at least 6 axes of data to function
    if (!detectedAcc || !detectedGyro) {
        printsys(aahrs, "failed to detect required sensors!");
        if (!detectedAcc) {
            printsys(aahrs, "missing: accelerometer");
        }
        if (!detectedGyro) {
            printsys(aahrs, "missing: gyroscope");
        }
        return false;
    }
    return true;
}

void fusion_deinit() {
    for (u32 i = 0; i < detectedCount; i++) {
        const FusionDevice *dev = detected[i];
        deinit_driver(dev, dev->acc, "accelerometer");
        deinit_driver(dev, dev->gyro, "gyroscope");
        deinit_driver(dev, dev->mag, "magnetometer");
        deinit_driver(dev, dev->baro, "barometer");
    }
    detectedCount = 0;
}
