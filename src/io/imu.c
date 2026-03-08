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
#include "lib/fusion/calibration.h"
#include "lib/fusion/fusion.h"
#include "sys/configuration.h"
#include "sys/print.h"

#include "imu.h"

#define MAX_DEVICES 5                        // The maximum number of sensors that are supported at once
#define UPDATE_RATE 200                      // IMU update rate in Hz
#define UPDATE_INTERVAL (1000 / UPDATE_RATE) // Update interval in ms
#define GPS_YAW_MIN_SPEED_KTS 5.0f           // Minimum groundspeed before trusting GPS track for yaw correction

static FusionDevice *detected[MAX_DEVICES];
static u32 detectedCount = 0; // Number of successfully detected devices
static bool i2cInitialized = false;

// Sensor fusion state
static FusionConfig fusionConfig;
static FusionState fusionState;
static Timestamp lastUpdate;
// Runtime axis remap and sign conventions learned during calibration
static i8 axisMap[3] = {0, 1, 2};  // Default is X, Y, Z = roll, pitch, yaw
static i8 axisSign[3] = {1, 1, 1}; // Default signs are all positive

// Applies the learned axis remap to the input data, returns remapped output data
static void imu_apply_axis_map(const f32 input[3], f32 output[3]) {
    for (u32 i = 0; i < 3; i++) {
        output[i] = input[axisMap[i]];
    }
}

// Applies the learned axis sign conventions to the input data, returns signed output data
static void imu_apply_axis_sign(const f32 input[3], f32 output[3]) {
    for (u32 i = 0; i < 3; i++) {
        output[i] = input[i] * (f32)axisSign[i];
    }
}

/**
 * Reads data from all detected sensors and averages them together.
 * @param accel_avg output parameter for averaged acceleration data (g)
 * @param gyro_avg output parameter for averaged gyro data (deg/s)
 * @return true if at least one accel and one gyro reading were successfully read and averaged
 */
static bool imu_read_average_sensor_data(f32 accel_avg[3], f32 gyro_avg[3]) {
    f32 accelSum[3] = {0.f, 0.f, 0.f};
    f32 gyroSum[3] = {0.f, 0.f, 0.f};
    u32 accelCount = 0;
    u32 gyroCount = 0;
    // Read data from all detected sensors
    for (u32 i = 0; i < detectedCount; i++) {
        FusionDevice *dev = detected[i];
        if (dev->acc && dev->acc->read && dev->acc->read(dev->acc, dev->accData)) {
            accelSum[0] += dev->accData[0];
            accelSum[1] += dev->accData[1];
            accelSum[2] += dev->accData[2];
            accelCount++;
        }
        if (dev->gyro && dev->gyro->read && dev->gyro->read(dev->gyro, dev->gyroData)) {
            gyroSum[0] += dev->gyroData[0];
            gyroSum[1] += dev->gyroData[1];
            gyroSum[2] += dev->gyroData[2];
            gyroCount++;
        }
    }
    if (accelCount == 0 || gyroCount == 0) {
        return false;
    }
    // Average the data
    accel_avg[0] = accelSum[0] / (f32)accelCount;
    accel_avg[1] = accelSum[1] / (f32)accelCount;
    accel_avg[2] = accelSum[2] / (f32)accelCount;
    gyro_avg[0] = gyroSum[0] / (f32)gyroCount;
    gyro_avg[1] = gyroSum[1] / (f32)gyroCount;
    gyro_avg[2] = gyroSum[2] / (f32)gyroCount;
    return true;
}

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
    fusionConfig.useGPS = gps.is_supported(); // Enable GPS yaw correction if GPS is supported
    fusion_reset(&fusionState);
    fusion_attitude_calibration_reset_axis_remap(axisMap, axisSign);
    fusion_attitude_calibration_init(&fusionConfig, axisMap, axisSign);

    // Load calibration data if available
    if (calibration.imu[IMU_CALIBRATED]) {
        f32 gyroBias[3] = {calibration.imu[IMU_GYRO_BIAS_X], calibration.imu[IMU_GYRO_BIAS_Y],
                           calibration.imu[IMU_GYRO_BIAS_Z]};
        f32 accelOffset[3] = {calibration.imu[IMU_ACCEL_OFFSET_X], calibration.imu[IMU_ACCEL_OFFSET_Y],
                              calibration.imu[IMU_ACCEL_OFFSET_Z]};
        fusion_load_calibration(&fusionConfig, gyroBias, accelOffset);
        if (!fusion_attitude_calibration_load_axis_remap(axisMap, axisSign)) {
            printsys(imu, "no valid fusion axis remap found, using identity axes");
        }
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

    // Read raw sensor data and apply calibration offsets
    f32 accelRaw[3], gyroRaw[3];
    if (!imu_read_average_sensor_data(accelRaw, gyroRaw)) {
        return;
    }
    fusion_attitude_calibration_update(accelRaw, gyroRaw);
    // Remap axes according to learned calibration
    f32 accelMapped[3], gyroMapped[3];
    imu_apply_axis_map(accelRaw, accelMapped);
    imu_apply_axis_map(gyroRaw, gyroMapped);
    // Get GPS track for yaw correction if available
    f32 gpsTrack = NAN;
    if (fusionConfig.useGPS && aircraft_is_gps_safe() && gps.track >= 0 && gps.speed >= GPS_YAW_MIN_SPEED_KTS) {
        gpsTrack = gps.track;
    }

    // Update fusion
    fusion_update(&fusionConfig, &fusionState, accelMapped, gyroMapped, gpsTrack, time_since_s(&lastUpdate));

    // Sign conventions are applied after fusion
    // This avoids destabilizing roll/pitch accel math when gyro and accel sign conventions differ
    f32 fusedAngles[3] = {fusionState.roll, fusionState.pitch, fusionState.yaw};
    f32 fusedRates[3] = {fusionState.rollRate, fusionState.pitchRate, fusionState.yawRate};
    f32 remappedAngles[3] = {};
    f32 remappedRates[3] = {};
    imu_apply_axis_sign(fusedAngles, remappedAngles);
    imu_apply_axis_sign(fusedRates, remappedRates);

    // Update public IMU struct with remapped and signed fused data
    imu.roll = remappedAngles[0];
    imu.pitch = remappedAngles[1];
    imu.yaw = remappedAngles[2];
    imu.rollRate = remappedRates[0];
    imu.pitchRate = remappedRates[1];
    imu.yawRate = remappedRates[2];
    memcpy(imu.accel, accelMapped, sizeof(imu.accel));
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
    fusion_attitude_calibration_deinit();
    imu.ready = false;
    fusion_reset(&fusionState);
    aircraft_set_imu_safe(false);
}

IMUCalibrationStatus imu_calibrate() {
#if !SIMCONNECT
    if (!imu.ready || detectedCount == 0) {
        return IMU_CALIBRATION_FAILED;
    }
    IMUCalibrationStatus status = fusion_attitude_calibration_status();
    if (status == IMU_CALIBRATION_NOT_STARTED) {
        return fusion_attitude_calibration_start();
    }
    return status;
#else
    return IMU_CALIBRATION_FAILED;
#endif
}

IMU imu = {
    .init = imu_init,
    .deinit = imu_deinit,
    .update = imu_update,
    .calibrate = imu_calibrate,
};
