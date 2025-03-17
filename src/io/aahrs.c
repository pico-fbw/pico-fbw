/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <string.h>
#include "platform/helpers.h"
#include "platform/i2c.h"
#if SIMCONNECT
    #include "platform/simconnect.h"
#endif
#include "platform/time.h"

#include "ctrl/aircraft.h"
#include "lib/fusion/fusion.h"
#include "sys/configuration.h"
#include "sys/print.h"

#include "aahrs.h"

static bool i2cInitialized = false; // Whether the AAHRS I2C bus has already been initialized

bool aahrs_init() {
    // Set up the I2C bus
    if (!i2cInitialized) {
        if (!i2c_setup((i16)config.pins[PINS_AAHRS_SDA], (i16)config.pins[PINS_AAHRS_SCL],
                       (u32)config.sensors[SENSORS_AAHRS_BUS_FREQ] * 1000)) {
            printsys(aahrs, "failed to initialize I2C bus");
            return false;
        }
        i2cInitialized = true;
    }
    // Initialize fusion system (scan for/initialize devices, start filter algorithms, etc.)
    if (!fusion_init()) {
        printsys(aahrs, "failed to initialize fusion system");
        return false;
    }
    aahrs.ready = true;
    aircraft.set_aahrs_safe(true);
    return true;
}

void aahrs_deinit() {
    fusion_deinit();
    printsys(aahrs, "deinitialized fusion");
    aahrs.roll = 0.f;
    aahrs.pitch = 0.f;
    aahrs.yaw = 0.f;
    aahrs.rollRate = 0.f;
    aahrs.pitchRate = 0.f;
    aahrs.yawRate = 0.f;
    aahrs.accel[0] = 0.f;
    aahrs.accel[1] = 0.f;
    aahrs.accel[2] = 0.f;
    aahrs.alt = -1;
    aahrs.ready = false;
    aircraft.set_aahrs_safe(false);
}

void aahrs_update() {
#if !SIMCONNECT_AAHRS_SKIP_FUSION
    // ...
#else
    // Roll and pitch must be inverted as MSFS uses a different convention than pico-fbw
    aahrs.roll = -(f32)scIMU.roll;
    aahrs.pitch = -(f32)scIMU.pitch;
    aahrs.yaw = (f32)scIMU.yaw;
    aahrs.rollRate = -(f32)scIMU.gyro[0];
    aahrs.pitchRate = -(f32)scIMU.gyro[1];
    aahrs.yawRate = (f32)scIMU.gyro[2];
    memcpy(aahrs.accel, scIMU.accel, sizeof(aahrs.accel));
#endif // !SIMCONNECT_AAHRS_SKIP_FUSION
}

bool aahrs_calibrate() {
    // ...

    // Flag AAHRS as calibrated, note the models at time of calibration, and save
    calibration.aahrs[AAHRS_CALIBRATED] = true;
    config_save();
    return true;
}

AAHRS aahrs = {
    .isCalibrated = false, // Will be set by aahrs_init() as applicable
    .ready = false,
    .init = aahrs_init,
    .deinit = aahrs_deinit,
    .update = aahrs_update,
    .calibrate = aahrs_calibrate,
};
