/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "platform/i2c.h"
#if SIMCONNECT
    #include "platform/simconnect.h"
#endif

#include "ctrl/aircraft.h"
#include "lib/drivers/drivers.h"
#include "sys/configuration.h"
#include "sys/print.h"

#include "imu.h"

#define MAX_DEVICES 5 // The maximum number of sensors that are supported at once

static const FusionDevice *detected[MAX_DEVICES];
static u32 detectedCount = 0;
static bool i2cInitialized = false;

bool imu_init() {
    // Set up I2C bus
    if (!i2cInitialized) {
        if (!i2c_setup((i16)config.pins[PINS_I2C_SDA], (i16)config.pins[PINS_I2C_SCL],
                       (u32)config.sensors[SENSORS_I2C_BUS_FREQ] * 1000)) {
            printsys(imu, "failed to initialize I2C bus");
            return false;
        }
        i2cInitialized = true;
    }
    // Scan through all known sensors and attempt to init their drivers
    printsys(imu, "detecting sensors");
    bool detectedAcc = false, detectedGyro = false;
    for (u32 i = 0; i < numFusionDevices; i++) {
        const FusionDevice *dev = fusionDevices[i];
        bool acc = init_driver(dev, dev->acc, "accelerometer");
        bool gyro = init_driver(dev, dev->gyro, "gyroscope");
        bool mag = init_driver(dev, dev->mag, "magnetometer");
        bool baro = init_driver(dev, dev->baro, "barometer");
        if (acc || gyro || mag || baro) {
            printsys(imu, "successfully detected and initialized '%s'", dev->name);
            detected[detectedCount++] = dev;
            detectedAcc |= acc;
            detectedGyro |= gyro;
        }
    }
    // Sensor fusion needs at least 6 axes of data to function
    if (!detectedAcc || !detectedGyro) {
        printsys(imu, "failed to detect required sensors!");
        if (!detectedAcc) {
            printsys(imu, "missing: accelerometer");
        }
        if (!detectedGyro) {
            printsys(imu, "missing: gyroscope");
        }
        return false;
    }
    aircraft.set_imu_safe(true);
    return true;
}

void imu_update() {
#if !SIMCONNECT
    // ...
#else
    // Roll and pitch must be inverted as MSFS uses a different convention than pico-fbw
    imu.roll = -(f32)scIMU.roll;
    imu.pitch = -(f32)scIMU.pitch;
    imu.yaw = (f32)scIMU.yaw;
    imu.rollRate = -(f32)scIMU.gyro[0];
    imu.pitchRate = -(f32)scIMU.gyro[1];
    imu.yawRate = (f32)scIMU.gyro[2];
    memcpy(imu.accel, scIMU.accel, sizeof(imu.accel));
#endif // !SIMCONNECT
}

void imu_deinit() {
    // Deinitialize all sensors
    for (u32 i = 0; i < detectedCount; i++) {
        const FusionDevice *dev = detected[i];
        deinit_driver(dev, dev->acc, "accelerometer");
        deinit_driver(dev, dev->gyro, "gyroscope");
        deinit_driver(dev, dev->mag, "magnetometer");
        deinit_driver(dev, dev->baro, "barometer");
    }
    detectedCount = 0;
    aircraft.set_imu_safe(false);
}

IMU imu = {
    .init = imu_init,
    .deinit = imu_deinit,
    .update = imu_update,
};
