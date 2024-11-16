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
    dev->ax = (i16)(scIMU.accel[0] * SC_SCALE_FACTOR);
    dev->ay = (i16)(scIMU.accel[1] * SC_SCALE_FACTOR);
    dev->az = (i16)(scIMU.accel[2] * SC_SCALE_FACTOR);
    return true;
    (void)state;
}

bool simconnect_gyro_read(Gyroscope *dev, void *state) {
    dev->gx = (i16)(scIMU.gyro[0] * SC_SCALE_FACTOR);
    dev->gy = (i16)(scIMU.gyro[1] * SC_SCALE_FACTOR);
    dev->gz = (i16)(scIMU.gyro[2] * SC_SCALE_FACTOR);
    return true;
    (void)state;
}

#endif // SIMCONNECT
