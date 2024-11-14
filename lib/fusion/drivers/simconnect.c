/**
 * pico-fbw's IMU/fusion implementation is based on the mongoose-os's IMU library.
 * Check it out at https://github.com/mongoose-os-libs/imu
 */

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#if SIMCONNECT

// clang-format off

#include "platform/simconnect.h"

#include "simconnect.h"

// clang-format on

bool simconnect_detect(byte addr, void *state) {
    return simconnect_ready();
    (void)addr;
    (void)state;
}

bool simconnect_acc_read(Accelerometer *dev, void *state) {
    dev->ax = scIMU.accel[0];
    dev->ay = scIMU.accel[1];
    dev->az = scIMU.accel[2];
    return true;
    (void)state;
}

bool simconnect_gyro_read(Gyroscope *dev, void *state) {
    dev->gx = scIMU.gyro[0];
    dev->gy = scIMU.gyro[1];
    dev->gz = scIMU.gyro[2];
    return true;
    (void)state;
}

#endif // SIMCONNECT
