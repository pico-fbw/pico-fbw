/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <string.h>
#include "platform/helpers.h"
#include "platform/i2c.h"
#include "platform/time.h"

#include "lib/fusion/fusion.h"
#include "lib/fusion/madgwick.h"

#include "modes/aircraft.h"

#include "sys/configuration.h"
#include "sys/log.h"
#include "sys/print.h"

#include "aahrs.h"

static IMU *imu;
static Madgwick *filter;

// Sensor and fusion parameters
#define ACC_SCALE 16    // G
#define ACC_ODR 100     // Output data rate in Hz
#define GYRO_SCALE 2000 // deg/s
#define GYRO_ODR 100
#define MAG_SCALE 12 // gauss
#define MAG_ODR 100
#define FUSION_RATE 100 // Hz
#define FUSION_BETA 0.1 // Madgwick filter beta parameter
#define FUSION_CALIBRATION_SAMPLES 5000

// TODO: add magnetometer calibration to fusion (need this before it can be used in the filter)

bool aahrs_init() {
    // Check the state of any previous calibration
    aahrs.isCalibrated = (bool)calibration.aahrs[AAHRS_CALIBRATED];
    bool differentIMU = (IMUModel)calibration.aahrs[AAHRS_IMU_MODEL] != (IMUModel)config.sensors[SENSORS_IMU_MODEL];
    bool differentBaro = (BaroModel)calibration.aahrs[AAHRS_BARO_MODEL] != (BaroModel)config.sensors[SENSORS_BARO_MODEL];
    if (aahrs.isCalibrated && (differentIMU || differentBaro)) {
        printfbw(aahrs, "calibration was performed on different models, recalibration is necessary!");
        // This ensures the system won't load any bad calibration into the fusion algorithms
        aahrs.isCalibrated = false;
    }

    // Set up the I2C bus and scan for any supported sensors
    static bool i2cInitialized = false;
    if (!i2cInitialized) {
        i2c_setup((u32)config.pins[PINS_AAHRS_SDA], (u32)config.pins[PINS_AAHRS_SCL],
                  (u32)config.sensors[SENSORS_AAHRS_BUS_FREQ] * 1000);
        i2cInitialized = true;
    }
    imu = fusion_imu_create();
    if (imu == NULL) {
        printfbw(aahrs, "failed to create IMU instance");
        return false;
    }

    AccelerometerOptions accOpts;
    accOpts.scale = ACC_SCALE;
    accOpts.odr = ACC_ODR;
    accOpts.no_rst = false;
    if (!fusion_accelerometer_find(imu, &accOpts)) {
        printfbw(aahrs, "failed to create accelerometer instance");
        return false;
    }

    GyroscopeOptions gyroOpts;
    gyroOpts.scale = GYRO_SCALE; // deg/s
    gyroOpts.odr = GYRO_ODR;
    if (!fusion_gyroscope_find(imu, &gyroOpts)) {
        printfbw(aahrs, "failed to create gyroscope instance");
        return false;
    }

    MagnetometerOptions magOpts;
    magOpts.scale = MAG_SCALE; // gauss
    magOpts.odr = MAG_ODR;
    if (!fusion_magnetometer_find(imu, &magOpts)) {
        printfbw(aahrs, "failed to create magnetometer instance");
        return false;
    }

    // Set up the Madgwick filter
    filter = madgwick_create();
    madgwick_set_params(filter, FUSION_RATE, FUSION_BETA);
    // TODO: load calibration data once saving works

    aahrs.isInitialized = true;
    return true;
}

void aahrs_deinit() {
    printfbw(aahrs, "stopping!");
    aahrs.roll = INFINITY;
    aahrs.pitch = INFINITY;
    aahrs.yaw = INFINITY;
    aahrs.alt = -1;
    madgwick_destroy(&filter);
    fusion_imu_destroy(&imu);
    aahrs.isInitialized = false;
}

void aahrs_update() {
    static Timestamp lastUpdate;
    // Throttle the update rate
    if (time_since_s(&lastUpdate) < (1.f / FUSION_RATE))
        return;

    f32 acc[3], gyro[3], mag[3];
    fusion_accelerometer_get(imu, &acc[0], &acc[1], &acc[2]);
    fusion_gyroscope_get(imu, &gyro[0], &gyro[1], &gyro[2]);
    fusion_magnetometer_get(imu, &mag[0], &mag[1], &mag[2]);

    madgwick_update(filter, radians(gyro[0]), radians(gyro[1]), radians(gyro[2]), acc[0], acc[1], acc[2], 0.f, 0.f, 0.f);
    // FIXME: For when magnetometer calibration is added:
    // madgwick_update(filter, radians(gyro[0]), radians(gyro[1]), radians(gyro[2]), acc[0], acc[1], acc[2], mag[0],
    //                          mag[1], mag[2]);
    lastUpdate = timestamp_now();

    f32 roll, pitch, yaw;
    if (!madgwick_get_angles(filter, &roll, &pitch, &yaw)) {
        printfbw(aahrs, "failed to get angles");
        aircraft.set_aahrs_safe(false);
        return;
    }
    aahrs.roll = degrees(roll);
    aahrs.pitch = degrees(pitch);
    aahrs.yaw = degrees(yaw);
    // TODO: are the rates pulled from gyro confirmed to be same axes as angles?
    aahrs.rollRate = gyro[0];
    aahrs.pitchRate = gyro[1];
    aahrs.yawRate = gyro[2];
    memcpy(aahrs.accel, acc, sizeof(aahrs.accel));
}

bool aahrs_calibrate() {
    // TODO: this calibration is very primitive and should be improved
    // it assumes the IMU is perfectly parallel to the ground
    // also, drift detection should be added to check if the calibration actually works

    // Get offset values for the accelerometer and gyroscope
    f64 ao[3] = {0.0, 0.0, 0.0}, go[3] = {0.0, 0.0, 0.0};
    for (u32 i = 0; i < FUSION_CALIBRATION_SAMPLES; i++) {
        f32 a[3], g[3];
        fusion_accelerometer_get(imu, &a[0], &a[1], &a[2]);
        fusion_gyroscope_get(imu, &g[0], &g[1], &g[2]);
        ao[0] += a[0];
        ao[1] += a[1];
        ao[2] += a[2];
        go[0] += g[0];
        go[1] += g[1];
        go[2] += g[2];
    }
    ao[0] /= FUSION_CALIBRATION_SAMPLES;
    ao[1] /= FUSION_CALIBRATION_SAMPLES;
    ao[2] /= FUSION_CALIBRATION_SAMPLES;
    for (u32 i = 0; i < 3; i++) {
        // Probably gravity?
        if (ao[i] > 0.9)
            ao[i] -= 1;
    }
    go[0] /= FUSION_CALIBRATION_SAMPLES;
    go[1] /= FUSION_CALIBRATION_SAMPLES;
    go[2] /= FUSION_CALIBRATION_SAMPLES;
    printraw("ao: %.4f %.4f %.4f\n", ao[0], ao[1], ao[2]);
    printraw("go: %.4f %.4f %.4f\n", go[0], go[1], go[2]);
    // TODO: save
    fusion_accelerometer_set_offset(imu, -ao[0], -ao[1], -ao[2]);
    fusion_gyroscope_set_offset(imu, -go[0], -go[1], -go[2]);

    // Flag AAHRS as calibrated, note the models at time of calibration, and save
    calibration.aahrs[AAHRS_CALIBRATED] = true;
    calibration.aahrs[AAHRS_IMU_MODEL] = (IMUModel)config.sensors[SENSORS_IMU_MODEL];
    calibration.aahrs[AAHRS_BARO_MODEL] = (BaroModel)config.sensors[SENSORS_BARO_MODEL];
    config_save();
    return true;
}

AAHRS aahrs = {
    .roll = INFINITY,
    .pitch = INFINITY,
    .yaw = INFINITY,
    .rollRate = INFINITY,
    .pitchRate = INFINITY,
    .yawRate = INFINITY,
    .accel = {0.f, 0.f, 0.f},
    .alt = -1,
    .init = aahrs_init,
    .deinit = aahrs_deinit,
    .update = aahrs_update,
    .calibrate = aahrs_calibrate,
    .isCalibrated = false, // Will be set by aahrs_init() as applicable
    .isInitialized = false,
};
