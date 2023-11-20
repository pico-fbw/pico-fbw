
/*
 * Copyright (c) 2020, Bjarne Hansen
 * All rights reserved.
 *
 * This file utilizes code under the BSD-3-Clause License. See "LICENSE" for details.
*/

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

// https://github.com/BjarneBitscrambler/OrientationSensorFusion-ESP/wiki/Calibration#calibration

#include <stdio.h>
#include <string.h>

#include "../../io/flash.h"

#include "calibration.h"

#define CALIBRATION_BUF_MAGNETIC_START 0
#define CALIBRATION_BUF_MAGNETIC_HEADER_SIZE 1
#define CALIBRATION_BUF_MAGNETIC_HEADER_MAGIC 0.15567f
#define CALIBRATION_BUF_MAGNETIC_VAL_SIZE 16
#define CALIBRATION_BUF_MAGNETIC_VAL_SIZE_BYTES (CALIBRATION_BUF_MAGNETIC_VAL_SIZE * sizeof(float))
#define CALIBRATION_BUF_MAGNETIC_TOT_SIZE (CALIBRATION_BUF_MAGNETIC_HEADER_SIZE + CALIBRATION_BUF_MAGNETIC_VAL_SIZE)

#define CALIBRATION_BUF_GYRO_START (CALIBRATION_BUF_MAGNETIC_TOT_SIZE)
#define CALIBRATION_BUF_GYRO_HEADER_SIZE 1
#define CALIBRATION_BUF_GYRO_HEADER_MAGIC 0.37204f
#define CALIBRATION_BUF_GYRO_VAL_SIZE 3
#define CALIBRATION_BUF_GYRO_VAL_SIZE_BYTES (CALIBRATION_BUF_GYRO_VAL_SIZE * sizeof(float))
#define CALIBRATION_BUF_GYRO_TOT_SIZE (CALIBRATION_BUF_GYRO_HEADER_SIZE + CALIBRATION_BUF_GYRO_VAL_SIZE)

#define CALIBRATION_BUF_ACCEL_START (CALIBRATION_BUF_MAGNETIC_TOT_SIZE + CALIBRATION_BUF_GYRO_TOT_SIZE)
#define CALIBRATION_BUF_ACCEL_HEADER_SIZE 1
#define CALIBRATION_BUF_ACCEL_HEADER_MAGIC 0.81040f
#define CALIBRATION_BUF_ACCEL_VAL_SIZE 21
#define CALIBRATION_BUF_ACCEL_VAL_SIZE_BYTES (CALIBRATION_BUF_ACCEL_VAL_SIZE * sizeof(float))
#define CALIBRATION_BUF_ACCEL_TOT_SIZE (CALIBRATION_BUF_ACCEL_HEADER_SIZE + CALIBRATION_BUF_ACCEL_VAL_SIZE)

#if FUSION_CALIBRATION_STORAGE_SIZE < (CALIBRATION_BUF_MAGNETIC_TOT_SIZE + CALIBRATION_BUF_GYRO_TOT_SIZE + CALIBRATION_BUF_ACCEL_TOT_SIZE)
	#error Insufficient space allocated for AAHRS calibration buffer
#endif

float *calibration_mag = &flash.aahrs[CALIBRATION_BUF_MAGNETIC_START];
float *calibration_gyro = &flash.aahrs[CALIBRATION_BUF_GYRO_START];
float *calibration_accel = &flash.aahrs[CALIBRATION_BUF_ACCEL_START];

bool GetMagCalibrationFromFlash(float *cal_values) {
    #if F_USING_MAG
        if (cal_values == NULL || !MagCalibrationExists()) {
            return false;
        }
        // Calibration header is valid, read the calibration into provided destination
        memcpy(cal_values, &calibration_mag[CALIBRATION_BUF_MAGNETIC_HEADER_SIZE], CALIBRATION_BUF_MAGNETIC_VAL_SIZE_BYTES);
        return true;
    #else
        return false;
    #endif // F_USING_MAG
}

bool GetGyroCalibrationFromFlash(float *cal_values) {
    #if F_USING_GYRO
        if (cal_values == NULL || !GyroCalibrationExists()) {
            return false;
        }
        memcpy(cal_values, &calibration_gyro[CALIBRATION_BUF_GYRO_HEADER_SIZE], CALIBRATION_BUF_GYRO_VAL_SIZE_BYTES);
        return true;
    #else
        return false;
    #endif // F_USING_GYRO
}

bool GetAccelCalibrationFromFlash(float *cal_values) {
    #if F_USING_ACCEL
        if (cal_values == NULL || !AccelCalibrationExists()) {
            return false;
        }
        memcpy(cal_values, &calibration_accel[CALIBRATION_BUF_ACCEL_HEADER_SIZE], CALIBRATION_BUF_ACCEL_VAL_SIZE_BYTES);
        return true;
    #else
        return false;
    #endif // F_USING_ACCEL
}

void SaveMagCalibrationToFlash(SensorFusionGlobals *sfg) {
    #if F_USING_MAG
        // Copy data from sfg->MagCal to calibration_mag, starting past the header
        memcpy(&calibration_mag[CALIBRATION_BUF_MAGNETIC_HEADER_SIZE], &(sfg->MagCal), CALIBRATION_BUF_MAGNETIC_VAL_SIZE_BYTES);
        // Write flag to indicate complete calibration and save
        calibration_mag[0] = CALIBRATION_BUF_MAGNETIC_HEADER_MAGIC;
        flash_save();
    #else
        return;
    #endif
}

void SaveGyroCalibrationToFlash(SensorFusionGlobals *sfg) {
    #if F_USING_GYRO && (F_9DOF_GBY_KALMAN || F_6DOF_GY_KALMAN)
        // Obtain source of the data which is different per fusion algorithm
        float *src;
        #if F_9DOF_GBY_KALMAN
            src = &(sfg->SV_9DOF_GBY_KALMAN.fbPl[0]);
        #elif F_6DOF_GY_KALMAN
            src = &(sfg->SV_6DOF_GY_KALMAN.fbPl[0]);
        #endif
        memcpy(&calibration_gyro[CALIBRATION_BUF_GYRO_HEADER_SIZE], src, CALIBRATION_BUF_GYRO_VAL_SIZE_BYTES);
        calibration_gyro[0] = CALIBRATION_BUF_GYRO_HEADER_MAGIC;
        flash_save();
    #else
        return;
    #endif
}

void SaveAccelCalibrationToFlash(SensorFusionGlobals *sfg) {
    #if F_USING_ACCEL
        memcpy(&calibration_accel[CALIBRATION_BUF_ACCEL_HEADER_SIZE], &(sfg->AccelCal), CALIBRATION_BUF_ACCEL_VAL_SIZE_BYTES);
        calibration_accel[0] = CALIBRATION_BUF_ACCEL_HEADER_MAGIC;
        flash_save();
    #else
        return;
    #endif
}

bool MagCalibrationExists() {
    // The header of each calibration block type contains magic value if valid calibration has been previously stored
    return (calibration_mag[0] == CALIBRATION_BUF_MAGNETIC_HEADER_MAGIC);
}

bool GyroCalibrationExists() {
    return (calibration_gyro[0] == CALIBRATION_BUF_GYRO_HEADER_MAGIC);
}

bool AccelCalibrationExists() {
    return (calibration_accel[0] == CALIBRATION_BUF_ACCEL_HEADER_MAGIC);
}
