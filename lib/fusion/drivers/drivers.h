/*
 * Copyright 2018 Google Inc.
 *
 * This file utilizes code under the Apache-2.0 License. See "LICENSE" for details.
 */

/**
 * pico-fbw's IMU/fusion implementation is based on the mongoose-os's IMU library.
 * Check it out at https://github.com/mongoose-os-libs/imu
 */

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#pragma once

#include <stdbool.h>
#include "platform/int.h"

/* Accelerometer */

typedef struct Accelerometer Accelerometer;

typedef bool (*acc_detect_fn)(Accelerometer *dev, void *imu_user_data);
typedef bool (*acc_create_fn)(Accelerometer *dev, void *imu_user_data);
typedef bool (*acc_destroy_fn)(Accelerometer *dev, void *imu_user_data);
typedef bool (*acc_read_fn)(Accelerometer *dev, void *imu_user_data);
typedef bool (*acc_get_odr_fn)(Accelerometer *dev, void *imu_user_data, float *odr);
typedef bool (*acc_set_odr_fn)(Accelerometer *dev, void *imu_user_data, float odr);
typedef bool (*acc_get_scale_fn)(Accelerometer *dev, void *imu_user_data, float *scale);
typedef bool (*acc_set_scale_fn)(Accelerometer *dev, void *imu_user_data, float scale);

typedef struct AccelerometerOptions {
    float odr;   // Data rate, in Hz. See doc for set_odr().
    float scale; // Scale. See doc for set_scale().
    bool no_rst; // Do not perform reset of the device when configuring.
} AccelerometerOptions;

typedef struct Accelerometer {
    acc_detect_fn detect;
    acc_create_fn create;
    acc_destroy_fn destroy;
    acc_read_fn read;
    acc_get_odr_fn get_odr;
    acc_set_odr_fn set_odr;
    acc_get_scale_fn get_scale;
    acc_set_scale_fn set_scale;

    byte addr; // I2C address of the device
    AccelerometerOptions opts;

    float scale;
    float offset_ax, offset_ay, offset_az;
    int16_t ax, ay, az;
} Accelerometer;

/* Gyroscope */

typedef struct Gyroscope Gyroscope;

typedef bool (*gyro_detect_fn)(Gyroscope *dev, void *imu_user_data);
typedef bool (*gyro_create_fn)(Gyroscope *dev, void *imu_user_data);
typedef bool (*gyro_destroy_fn)(Gyroscope *dev, void *imu_user_data);
typedef bool (*gyro_read_fn)(Gyroscope *dev, void *imu_user_data);
typedef bool (*gyro_get_odr_fn)(Gyroscope *dev, void *imu_user_data, float *odr);
typedef bool (*gyro_set_odr_fn)(Gyroscope *dev, void *imu_user_data, float odr);
typedef bool (*gyro_get_scale_fn)(Gyroscope *dev, void *imu_user_data, float *scale);
typedef bool (*gyro_set_scale_fn)(Gyroscope *dev, void *imu_user_data, float scale);

typedef struct GyroscopeOptions {
    float odr;   // Data rate, in Hz. See doc for set_odr().
    float scale; // Scale. See doc for set_scale().
    bool no_rst; // Do not perform reset of the device when configuring.
} GyroscopeOptions;

typedef struct Gyroscope {
    gyro_detect_fn detect;
    gyro_create_fn create;
    gyro_destroy_fn destroy;
    gyro_read_fn read;
    gyro_get_odr_fn get_odr;
    gyro_set_odr_fn set_odr;
    gyro_get_scale_fn get_scale;
    gyro_set_scale_fn set_scale;

    byte addr;
    GyroscopeOptions opts;

    float scale;
    float offset_gx, offset_gy, offset_gz;
    float orientation[9];
    int16_t gx, gy, gz;
} Gyroscope;

/* Magnetometer */

typedef struct Magnetometer Magnetometer;

typedef bool (*mag_detect_fn)(Magnetometer *dev, void *imu_user_data);
typedef bool (*mag_create_fn)(Magnetometer *dev, void *imu_user_data);
typedef bool (*mag_destroy_fn)(Magnetometer *dev, void *imu_user_data);
typedef bool (*mag_read_fn)(Magnetometer *dev, void *imu_user_data);
typedef bool (*mag_get_odr_fn)(Magnetometer *dev, void *imu_user_data, float *odr);
typedef bool (*mag_set_odr_fn)(Magnetometer *dev, void *imu_user_data, float odr);
typedef bool (*mag_get_scale_fn)(Magnetometer *dev, void *imu_user_data, float *scale);
typedef bool (*mag_set_scale_fn)(Magnetometer *dev, void *imu_user_data, float scale);

typedef struct MagnetometerOptions {
    float odr;   // Data rate, in Hz. See doc for set_odr().
    float scale; // Scale. See doc for set_scale().
    bool no_rst; // Do not perform reset of the device when configuring.
} MagnetometerOptions;

typedef struct Magnetometer {
    mag_detect_fn detect;
    mag_create_fn create;
    mag_destroy_fn destroy;
    mag_read_fn read;
    mag_get_odr_fn get_odr;
    mag_set_odr_fn set_odr;
    mag_get_scale_fn get_scale;
    mag_set_scale_fn set_scale;

    byte addr;
    MagnetometerOptions opts;

    float scale;
    float bias[3];
    float orientation[9];
    int16_t mx, my, mz;
} Magnetometer;

typedef struct IMU {
    Accelerometer *acc;
    Gyroscope *gyro;
    Magnetometer *mag;
    void *state; // A driver may choose to store some state here
} IMU;
