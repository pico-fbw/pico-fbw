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
#include "sys/configuration.h"
#include "sys/print.h"

#include "aahrs.h"

// TODO: redo the entire fusion system (sigh)

// TODO: https://ardupilot.org/copter/docs/deadreckoning-failsafe.html seems like an interesting feature to implement

static bool i2cInitialized = false; // Whether the AAHRS I2C bus has already been initialized

bool aahrs_init() {
    // Check the state of any previous calibration
    aahrs.isCalibrated = (bool)calibration.aahrs[AAHRS_CALIBRATED];
    bool differentIMU = (IMUModel)calibration.aahrs[AAHRS_IMU_MODEL] != (IMUModel)config.sensors[SENSORS_IMU_MODEL];
    bool differentBaro =
        (BaroModel)calibration.aahrs[AAHRS_BARO_MODEL] != (BaroModel)config.sensors[SENSORS_BARO_MODEL];
    if (aahrs.isCalibrated && (differentIMU || differentBaro)) {
        printsys(aahrs, "calibration was performed on different models, recalibration will be necessary!");
        // This ensures the system won't load any bad calibration into the fusion algorithms
        aahrs.isCalibrated = false;
    }

    // Set up the I2C bus and scan for any supported sensors
    if (!i2cInitialized) {
        i2c_setup((i16)config.pins[PINS_AAHRS_SDA], (i16)config.pins[PINS_AAHRS_SCL],
                  (u32)config.sensors[SENSORS_AAHRS_BUS_FREQ] * 1000);
        i2cInitialized = true;
    }

    // ...

    aahrs.isInitialized = true;
    aircraft.set_aahrs_safe(true);
    return true;
}

void aahrs_deinit() {
    printsys(aahrs, "stopping!");
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
    // ...
    aahrs.isInitialized = false;
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
    calibration.aahrs[AAHRS_IMU_MODEL] = (IMUModel)config.sensors[SENSORS_IMU_MODEL];
    calibration.aahrs[AAHRS_BARO_MODEL] = (BaroModel)config.sensors[SENSORS_BARO_MODEL];
    config_save();
    return true;
}

AAHRS aahrs = {
    .roll = 0.f,
    .pitch = 0.f,
    .yaw = 0.f,
    .rollRate = 0.f,
    .pitchRate = 0.f,
    .yawRate = 0.f,
    .accel = {0.f, 0.f, 0.f},
    .alt = -1,
    .init = aahrs_init,
    .deinit = aahrs_deinit,
    .update = aahrs_update,
    .calibrate = aahrs_calibrate,
    .isCalibrated = false, // Will be set by aahrs_init() as applicable
    .isInitialized = false,
};
