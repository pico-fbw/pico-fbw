#pragma once

#include <stdbool.h>
#include "platform/types.h"

#include "drivers/drivers.h"

#define NOADDR 0xFF // Denotes no address (for devices with only one I2C address)

typedef bool (*detect_fn)(byte addr, void *state);
typedef void *(*create_state_fn)();
typedef void *(*destroy_state_fn)(void *state);

typedef struct AccelerometerDetails {
    const char *name;     // Human-readable identifier of the sensor
    byte addr[2];         // Up to two possible I2C addresses
    Accelerometer device; // Device-specific functions

    detect_fn detect;               // Function to detect the sensor
    create_state_fn create_state;   // Optional function to create a sensor state
    destroy_state_fn destroy_state; // Optional function to destroy a sensor state
} AccelerometerDetails;

typedef struct GyroscopeDetails {
    const char *name;
    byte addr[2];
    Gyroscope device;

    detect_fn detect;
    create_state_fn create_state;
    destroy_state_fn destroy_state;
} GyroscopeDetails;

typedef struct MagnetometerDetails {
    const char *name;
    byte addr[2];
    Magnetometer device;

    detect_fn detect;
    create_state_fn create_state;
    destroy_state_fn destroy_state;
} MagnetometerDetails;

IMU *fusion_imu_create(void);
void fusion_imu_destroy(IMU **imu);

// Return true if the specified sensor is present in the system
bool fusion_accelerometer_present(IMU *imu);
bool fusion_gyroscope_present(IMU *imu);
bool fusion_magnetometer_present(IMU *imu);

/* Accelerometer functions, see accel.c */

// Scans the I2C bus for a supported accelerometer and initializes one if found
bool fusion_accelerometer_find(IMU *imu, const AccelerometerOptions *opts);

// Return accelerometer data in units of G
bool fusion_accelerometer_get(IMU *imu, f32 *x, f32 *y, f32 *z);

// Get/set accelerometer offset in units of G
bool fusion_accelerometer_get_offset(IMU *imu, f32 *x, f32 *y, f32 *z);
bool fusion_accelerometer_set_offset(IMU *imu, f32 x, f32 y, f32 z);

// Get/set accelerometer scale in units of G
// The driver will set the scale to at least the given `scale` parameter, eg 20 m/s/s
// Will return true upon success, false if setting the scale is not feasible.
bool fusion_accelerometer_get_scale(IMU *imu, f32 *scale);
bool fusion_accelerometer_set_scale(IMU *imu, f32 scale);

// Get/set accelerometer output data rate in units Hertz
// The driver will set the data rate to at least the given `hertz` parameter, eg 100
// Will return true upon success, false if setting the data rate is not feasible.
bool fusion_accelerometer_get_odr(IMU *imu, f32 *hertz);
bool fusion_accelerometer_set_odr(IMU *imu, f32 hertz);

/* Gyroscope functions, see gyro.c */

// Scans the I2C bus for a supported gyroscope and initializes one if found
bool fusion_gyroscope_find(IMU *imu, const GyroscopeOptions *opts);

// Return gyroscope data in units of degrees/sec
bool fusion_gyroscope_get(IMU *imu, f32 *x, f32 *y, f32 *z);

// Get/set gyroscope offset in units of degrees/sec
bool fusion_gyroscope_get_offset(IMU *imu, f32 *x, f32 *y, f32 *z);
bool fusion_gyroscope_set_offset(IMU *imu, f32 x, f32 y, f32 z);

// Get/set gyroscope scale in units of degrees/sec
// The driver will set the scale to at least the given `scale` parameter, eg 1000
// Will return true upon success, false if setting the scale is not feasible.
bool fusion_gyroscope_get_scale(IMU *imu, f32 *scale);
bool fusion_gyroscope_set_scale(IMU *imu, f32 scale);

// Get/set gyroscope output data rate in units Hertz
// The driver will set the data rate to at least the given `hertz` parameter, eg 100
// Will return true upon success, false if setting the data rate is not feasible.
bool fusion_gyroscope_get_odr(IMU *imu, f32 *hertz);
bool fusion_gyroscope_set_odr(IMU *imu, f32 hertz);

// Get/set gyroscope axes orientation relatve to accelerometer
// *vector is a list of 9 f32s which determine how much of a certain sensor axis
// should be blended into calls to fusion_gyroscope_get().
// Examples: swap x/y axes, and flip direction of z-axis
// f32 v[9]={0, 1, 0,     // x gets 0% of x-sensor, 100% of y-sensor, 0% of z-sensor
//             1, 0, 0,     // y gets 100% of x-sensor, 0% of y-sensor, 0% of z-sensor
//             0, 0, -1};   // z gets 0% of x-sensor, 0% of y-sensor, -100% of z-sensor
bool fusion_gyroscope_get_orientation(IMU *imu, f32 v[9]);
bool fusion_gyroscope_set_orientation(IMU *imu, f32 v[9]);

/* Magnetometer functions, see mag.c */

// Scans the I2C bus for a supported magnetometer and initializes one if found
bool fusion_magnetometer_find(IMU *imu, const MagnetometerOptions *opts);

// Return magnetometer data in units of Gauss
bool fusion_magnetometer_get(IMU *imu, f32 *x, f32 *y, f32 *z);

// Get/set magnetometer scale in units of Gauss
// The driver will set the scale to at least the given `scale` parameter, eg 400
// Will return true upon success, false if setting the scale is not feasible.
bool fusion_magnetometer_get_scale(IMU *imu, f32 *scale);
bool fusion_magnetometer_set_scale(IMU *imu, f32 scale);

// Get/set magnetometer output data rate in units Hertz
// The driver will set the data rate to at least the given `hertz` parameter, eg 100
// Will return true upon success, false if setting the data rate is not feasible.
bool fusion_magnetometer_get_odr(IMU *imu, f32 *hertz);
bool fusion_magnetometer_set_odr(IMU *imu, f32 hertz);

// Get/set magnetometer axes orientation relative to accelerometer
// *vector is a list of 9 f32s which determine how much of a certain sensor axis
// should be blended into calls to fusion_magnetometer_get().
// Examples: swap x/y axes, and flip direction of z-axis
// f32 v[9]={0, 1, 0,     // x gets 0% of x-sensor, 100% of y-sensor, 0% of z-sensor
//             1, 0, 0,     // y gets 100% of x-sensor, 0% of y-sensor, 0% of z-sensor
//             0, 0, -1};   // z gets 0% of x-sensor, 0% of y-sensor, -100% of z-sensor
bool fusion_magnetometer_get_orientation(IMU *imu, f32 v[9]);
bool fusion_magnetometer_set_orientation(IMU *imu, f32 v[9]);
