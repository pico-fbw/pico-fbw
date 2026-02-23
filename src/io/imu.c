/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <math.h>
#include <string.h>
#include "platform/i2c.h"
#include "platform/time.h"
#if SIMCONNECT
    #include "platform/simconnect.h"
#endif

#include "ctrl/aircraft.h"
#include "io/gps.h"
#include "lib/drivers/drivers.h"
#include "lib/fusion.h"
#include "sys/configuration.h"
#include "sys/print.h"

#include "imu.h"

#define MAX_DEVICES 5                        // The maximum number of sensors that are supported at once
#define UPDATE_RATE 200                      // IMU update rate in Hz
#define UPDATE_INTERVAL (1000 / UPDATE_RATE) // Update interval in ms
#define CALIBRATION_SAMPLES 1000             // Number of samples for calibration
#define GPS_YAW_MIN_SPEED_KTS 5.0f           // Minimum groundspeed before trusting GPS track for yaw correction

static FusionDevice *detected[MAX_DEVICES];
static u32 detectedCount = 0;
static bool i2cInitialized = false;

// Sensor fusion state
static FusionConfig fusionConfig;
static FusionState fusionState;
static FusionCalibration fusionCal;
static Timestamp lastUpdate;

// TODO: refactor to be more modular/less confusing in general

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
        FusionDevice *dev = fusionDevices[i];
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

    // Initialize sensor fusion
    fusion_init(&fusionConfig, UPDATE_RATE);
    // Enable GPS yaw correction if GPS is supported
    fusionConfig.useGPS = gps.is_supported();
    fusion_reset(&fusionState);

    // Load calibration data if available
    if (calibration.imu[IMU_CALIBRATED]) {
        f32 gyroBias[3] = {calibration.imu[IMU_GYRO_BIAS_X], calibration.imu[IMU_GYRO_BIAS_Y],
                           calibration.imu[IMU_GYRO_BIAS_Z]};
        f32 accelOffset[3] = {calibration.imu[IMU_ACCEL_OFFSET_X], // Already in g units
                              calibration.imu[IMU_ACCEL_OFFSET_Y], calibration.imu[IMU_ACCEL_OFFSET_Z]};
        fusion_load_calibration(&fusionConfig, gyroBias, accelOffset);
        imu.isCalibrated = true;
        printsys(imu, "loaded calibration data");
    } else {
        imu.isCalibrated = false;
        printsys(imu, "no calibration data found");
    }

    lastUpdate = timestamp_now();
    imu.ready = true;
    aircraft_set_imu_safe(true);
    return true;
}

void imu_update() {
#if !SIMCONNECT
    if (time_since_ms(&lastUpdate) < UPDATE_INTERVAL) {
        return; // Not time to update yet
    }

    // Read sensor data from all detected devices
    f32 accelSum[3] = {};
    f32 gyroSum[3] = {};
    u32 accelCount = 0, gyroCount = 0;
    for (u32 i = 0; i < detectedCount; i++) {
        FusionDevice *dev = detected[i];
        // Read accel data
        if (dev->acc && dev->acc->read) {
            if (dev->acc->read(dev->acc, dev->accData)) {
                accelSum[0] += dev->accData[0];
                accelSum[1] += dev->accData[1];
                accelSum[2] += dev->accData[2];
                accelCount++;
            }
        }
        // Read gyro data
        if (dev->gyro && dev->gyro->read) {
            if (dev->gyro->read(dev->gyro, dev->gyroData)) {
                gyroSum[0] += dev->gyroData[0];
                gyroSum[1] += dev->gyroData[1];
                gyroSum[2] += dev->gyroData[2];
                gyroCount++;
            }
        }
    }
    if (accelCount < 0 || gyroCount < 0) {
        return; // No valid sensor data read
    }
    // Average sensor readings
    f32 accelAvg[3] = {accelSum[0] / accelCount, accelSum[1] / accelCount, accelSum[2] / accelCount};
    f32 gyroAvg[3] = {gyroSum[0] / gyroCount, gyroSum[1] / gyroCount, gyroSum[2] / gyroCount};

    // Get GPS track for yaw correction if available
    f32 gpsTrack = NAN;
    if (fusionConfig.useGPS && aircraft_is_gps_safe() && gps.track >= 0 && gps.speed >= GPS_YAW_MIN_SPEED_KTS) {
        gpsTrack = gps.track;
    }
    // Update fusion
    fusion_update(&fusionConfig, &fusionState, accelAvg, gyroAvg, gpsTrack, time_since_s(&lastUpdate));

    // Copy data to IMU struct
    imu.roll = fusionState.roll;
    imu.pitch = fusionState.pitch;
    imu.yaw = fusionState.yaw;
    imu.rollRate = fusionState.rollRate;
    imu.pitchRate = fusionState.pitchRate;
    imu.yawRate = fusionState.yawRate;
    memcpy(imu.accel, accelAvg, sizeof(imu.accel));
    lastUpdate = timestamp_now();
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
    // TODO: proper accel calibration (multiple orientations?)
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
    imu.ready = false;
    fusion_reset(&fusionState);
    aircraft_set_imu_safe(false);
}

bool imu_calibrate() {
    printsys(imu, "starting IMU calibration - keep device level and stationary");
    fusion_calibration_start(&fusionCal);
    // Collect calibration samples
    for (u32 sample = 0; sample < CALIBRATION_SAMPLES; sample++) {
        // Read sensor data from all detected devices
        f32 accelSum[3] = {0, 0, 0};
        f32 gyroSum[3] = {0, 0, 0};
        u32 accelCount = 0, gyroCount = 0;
        for (u32 i = 0; i < detectedCount; i++) {
            const FusionDevice *dev = detected[i];
            // Read accel data
            if (dev->acc && dev->acc->read) {
                if (dev->acc->read(dev->acc, (f32 *)dev->accData)) {
                    accelSum[0] += dev->accData[0];
                    accelSum[1] += dev->accData[1];
                    accelSum[2] += dev->accData[2];
                    accelCount++;
                }
            }
            // Read gyro data
            if (dev->gyro && dev->gyro->read) {
                if (dev->gyro->read(dev->gyro, (f32 *)dev->gyroData)) {
                    gyroSum[0] += dev->gyroData[0];
                    gyroSum[1] += dev->gyroData[1];
                    gyroSum[2] += dev->gyroData[2];
                    gyroCount++;
                }
            }
        }

        if (accelCount > 0 && gyroCount > 0) {
            f32 accelAvg[3] = {accelSum[0] / accelCount, accelSum[1] / accelCount, accelSum[2] / accelCount};
            f32 gyroAvg[3] = {gyroSum[0] / gyroCount, gyroSum[1] / gyroCount, gyroSum[2] / gyroCount};
            fusion_calibration_add_sample(&fusionCal, accelAvg, gyroAvg);
        }
        sleep_ms_blocking(10); // Small delay between samples
        // Print progress every 100 samples
        if ((sample + 1) % 100 == 0) {
            printsys(imu, "calibration progress: %lu/%d", sample + 1, CALIBRATION_SAMPLES);
        }
    }

    // Finish calibration and save results
    if (fusion_calibration_finish(&fusionCal, &fusionConfig)) {
        // Store calibration data in config
        calibration.imu[IMU_CALIBRATED] = true;
        // Store gyro bias (deg/s)
        calibration.imu[IMU_GYRO_BIAS_X] = fusionConfig.gyroBias[0];
        calibration.imu[IMU_GYRO_BIAS_Y] = fusionConfig.gyroBias[1];
        calibration.imu[IMU_GYRO_BIAS_Z] = fusionConfig.gyroBias[2];
        // Store accel offset (already in g)
        calibration.imu[IMU_ACCEL_OFFSET_X] = fusionConfig.accelOffset[0];
        calibration.imu[IMU_ACCEL_OFFSET_Y] = fusionConfig.accelOffset[1];
        calibration.imu[IMU_ACCEL_OFFSET_Z] = fusionConfig.accelOffset[2];

        imu.isCalibrated = true;
        printsys(imu, "calibration successful!");
        printsys(imu, "gyro bias: [%.4f, %.4f, %.4f] deg/s", calibration.imu[IMU_GYRO_BIAS_X],
                 calibration.imu[IMU_GYRO_BIAS_Y], calibration.imu[IMU_GYRO_BIAS_Z]);
        printsys(imu, "accel offset: [%.4f, %.4f, %.4f] g", calibration.imu[IMU_ACCEL_OFFSET_X],
                 calibration.imu[IMU_ACCEL_OFFSET_Y], calibration.imu[IMU_ACCEL_OFFSET_Z]);
        return true;
    } else {
        printsys(imu, "calibration failed!");
        return false;
    }
}

IMU imu = {
    .init = imu_init,
    .deinit = imu_deinit,
    .update = imu_update,
    .calibrate = imu_calibrate,
};
