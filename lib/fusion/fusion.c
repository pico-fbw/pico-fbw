/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "sys/print.h"

#include "drivers/drivers.h"

#include "fusion.h"

#define MAX_DEVICES 5 // The maximum number of devices that are supported at once

static const FusionDevice *detected[MAX_DEVICES];
static u32 detectedCount = 0;

bool fusion_init() {
    // Scan through all known devices and attempt to init their drivers
    printsys(aahrs, "detecting fusion devices");
    bool detectedAcc = false, detectedGyro = false;
    for (u32 i = 0; i < numFusionDevices; i++) {
        const FusionDevice *dev = fusionDevices[i];
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
