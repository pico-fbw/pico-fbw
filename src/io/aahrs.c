/*
 * Copyright (c) 2020-2021, Bjarne Hansen
 * All rights reserved.
 *
 * This file utilizes code under the BSD-3-Clause License. See "LICENSE" for details.
 */

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "platform/time.h"

#include "io/display.h"

#include "lib/fusion/acc.h"
#include "lib/fusion/calibration.h"
#include "lib/fusion/fconfig.h"
#include "lib/fusion/fusion.h"
#include "lib/fusion/mag.h"
#include "lib/fusion/status.h"

#include "lib/fusion/drivers/ak09916.h"
#include "lib/fusion/drivers/bno055.h"
#include "lib/fusion/drivers/drivers.h"
#include "lib/fusion/drivers/icm20948.h"

#include "modes/aircraft.h"

#include "sys/configuration.h"
#include "sys/log.h"
#include "sys/print.h"
#include "sys/runtime.h"

#include "aahrs.h"

// The number of samples to average when calculating the gyroscope offset
#define GYRO_AVG_SAMPLES 100
// The maximum velocity (in deg/s) under which the gyroscope is considered to be stationary
#define GYRO_STILL_VELOCITY 3
// The maximum number of attempts to get a magnetometer calibration using the 10 element (best) solver
// The best current calibration will be used if the magnetometer is not calibrated after this many attempts, and a warning will
// be generated
#define MAX_MAG_ATTEMPTS 15

// Prints a vector (array)
#define PRINT_VECTOR(vector)                                                                                                   \
    do {                                                                                                                       \
        for (u32 i = 0; i < count_of(vector); i++) {                                                                           \
            printraw("%f ", vector[i]);                                                                                        \
        }                                                                                                                      \
        printraw("\n");                                                                                                        \
    } while (0)

// Prints a matrix (2-dimensional array)
#define PRINT_MATRIX(matrix)                                                                                                   \
    do {                                                                                                                       \
        for (u32 i = 0; i < count_of(matrix); i++) {                                                                           \
            for (u32 j = 0; j < count_of(matrix[0]); j++) {                                                                    \
                printraw("%f ", matrix[i][j]);                                                                                 \
            }                                                                                                                  \
            printraw("\n");                                                                                                    \
        }                                                                                                                      \
    } while (0)

static SensorFusionGlobals fusion;
static StatusSubsystem status;
struct PhysicalSensor *sensors;
static u32 numSensors = 0;
static u32 rateDelay = 0;   // Delay between fusion algorithm runs in microseconds
static Timestamp ranFusion; // Time of the last fusion algorithm run

bool aahrs_init() {
    // Check the state of any previous calibration
    aahrs.isCalibrated = (bool)calibration.aahrs[AAHRS_CALIBRATED];
    if (aahrs.isCalibrated &&
        ((IMUModel)calibration.aahrs[AAHRS_IMU_MODEL] != (IMUModel)config.sensors[SENSORS_IMU_MODEL] ||
         (BaroModel)calibration.aahrs[AAHRS_BARO_MODEL] != (BaroModel)config.sensors[SENSORS_BARO_MODEL])) {
        printfbw(aahrs, "calibration was performed on different models, recalibration is necessary!");
        // This ensures the system won't load any bad calibration into the fusion algorithms
        EraseFusionCalibration();
        aahrs.isCalibrated = false;
    }
    // Initialize (memory of) fusion systems
    initializeStatusSubsystem(&status);
    initSensorFusionGlobals(&fusion, &status);
    printfbw(aahrs, "initialized fusion memory defaults");
    // Install the sensors defined in the config
    if (shouldPrint.aahrs)
        printraw("[aahrs] installing ");
    switch ((IMUModel)config.sensors[SENSORS_IMU_MODEL]) {
        case IMU_MODEL_BNO055:
            // Allocate memory for the sensor and install it
            if (shouldPrint.aahrs)
                printraw("BNO055 ");
            numSensors++;
            sensors = reallocarray(sensors, numSensors, sizeof(struct PhysicalSensor));
            if (!sensors)
                return false;
            fusion.installSensor(&fusion, &sensors[numSensors - 1], BNO055_I2C_ADDR_LOW, 1, NULL, BNO055_init, BNO055_read);
            break;
        case IMU_MODEL_ICM20948:
            // The ICM20948 actually has an AK09916 inside it, but it has a different i2c address so we see it as a seperate
            // device
            // FIXME: AK09916 is broken after hal update for some reason?
            /*
            if (shouldPrint.aahrs)
                printraw("AK09916 ");
            numSensors++;
            sensors = reallocarray(sensors, numSensors, sizeof(struct PhysicalSensor));
            if (!sensors)
                return false;
            fusion.installSensor(&fusion, &sensors[numSensors - 1], AK09916_I2C_ADDR, 1, NULL, AK09916_init, AK09916_read);
            */
            if (shouldPrint.aahrs)
                printraw(", ICM20948 ");
            numSensors++;
            sensors = reallocarray(sensors, numSensors, sizeof(struct PhysicalSensor));
            if (!sensors)
                return false;
            fusion.installSensor(&fusion, &sensors[numSensors - 1], ICM20948_I2C_ADDR_HIGH, 1, NULL, ICM20948_init,
                                 ICM20948_read);
            break;
        default:
            printfbw(aahrs, "ERROR: unknown IMU model!");
            return false;
    }
    switch ((BaroModel)config.sensors[SENSORS_BARO_MODEL]) {
        case BARO_MODEL_NONE:
            break;
        case BARO_MODEL_DPS310:
            if (shouldPrint.aahrs)
                printraw(", DPS310 ");
            // Code incomplete
            break;
        default:
            printfbw(aahrs, "ERROR: unknown baro model!");
            return false;
    }
    if (shouldPrint.aahrs)
        printraw("\n");
    printfbw(aahrs, "initializing core fusion engine");
    fusion.initializeFusionEngine(&fusion);
    // Ensure initialization has been good so far
    if (fusion.getStatus(&fusion) != NORMAL)
        return false;
// Calculate the delay needed between fusion updates to obtain FUSION_HZ
#if F_9DOF_GBY_KALMAN
    rateDelay = (u32)((1.0f / FUSION_HZ) * 1E6f - (F_9DOF_GBY_KALMAN_SYSTICK + F_CONDITION_SENSOR_READINGS_SYSTICK));
#endif
    printfbw(aahrs, "[AAHRS] to obtain update rate of %dHz, using delay of %luus", FUSION_HZ, rateDelay);
    return true;
}

void aahrs_deinit() {
    printfbw(aahrs, "stopping!");
    aahrs.roll = INFINITY;
    aahrs.pitch = INFINITY;
    aahrs.yaw = INFINITY;
    aahrs.alt = -1;
}

void aahrs_update() {
    fusion.readSensors(&fusion, 0);
    // Only run fusion when we've waited for the rateDelay (essentially throttles algorithm)
    if (time_since_us(&ranFusion) >= rateDelay) {
        fusion.conditionSensorReadings(&fusion);
        fusion.runFusion(&fusion);
        // Use a common pointer that will be usable no matter which fusion mode is running
        SV_ptr common;
#if F_9DOF_GBY_KALMAN
        common = (SV_ptr)&fusion.SV_9DOF_GBY_KALMAN;
#endif
        aahrs.roll = common->fPhi;
        aahrs.rollRate = common->fOmega[CHX];
        aahrs.pitch = common->fThe;
        aahrs.pitchRate = common->fOmega[CHY];
        aahrs.yaw = common->fPsi;
        aahrs.yawRate = common->fOmega[CHZ];
        // Alt will go here, not done yet
        ranFusion = timestamp_now();
    }
}

bool aahrs_calibrate() {
    Timestamp wait;
    bool hasMoved = false;

// GYRO: Wait for gyro to stabilize, calculate its offsets a number of times, then average them and save
#if F_USING_GYRO && (F_9DOF_GBY_KALMAN || F_6DOF_GY_KALMAN)
    log_message(INFO, "Please hold still!", 1000, 200, true);
    if (runtime_is_fbw())
        display_string("Please hold still!", 0);
    // Wait for gyro (and user) to stabilize
    wait = timestamp_in_ms(5000);
    while (!timestamp_reached(&wait))
        aahrs.update();
    // Take GYRO_AVG_SAMPLES offsets
    float offsets[3];
    for (u32 i = 0; i < GYRO_AVG_SAMPLES; i++) {
        if (runtime_is_fbw())
            display_string("Please hold still!", (i + 1) * (33.0f / GYRO_AVG_SAMPLES));
    // Reqest a reset from the algorithm so that the gyro offsets get calculated
    #if F_9DOF_GBY_KALMAN
        fusion.SV_9DOF_GBY_KALMAN.resetflag = true;
    #elif F_6DOF_GY_KALMAN
        fusion.SV_6DOF_GY_KALMAN.resetflag = true;
    #endif
        // Wait for the calculation and save
        wait = timestamp_in_ms(rateDelay);
        while (!timestamp_reached(&wait))
            aahrs.update();
    #if F_9DOF_GBY_KALMAN
        for (u32 j = 0; j < count_of(offsets); j++)
            offsets[j] += fusion.SV_9DOF_GBY_KALMAN.fbPl[j];
    #elif F_6DOF_GY_KALMAN
        for (u32 j = 0; j < count_of(offsets); j++)
            offsets[j] += fusion.SV_6DOF_GY_KALMAN.fbPl[j];
    #endif
    }
    // Average the offsets and save
    for (u32 i = 0; i < count_of(offsets); i++) {
    #if F_9DOF_GBY_KALMAN
        fusion.SV_9DOF_GBY_KALMAN.fbPl[i] = offsets[i] / GYRO_AVG_SAMPLES;
    #elif F_6DOF_GY_KALMAN
        fusion.SV_6DOF_GY_KALMAN.fbPl[i] = offsets[i] / GYRO_AVG_SAMPLES;
    #endif
    }
    printfbw(aahrs, "saving gyro calibration");
    if (shouldPrint.aahrs) {
        printraw("\noffset vector:");
    #if F_9DOF_GBY_KALMAN
        PRINT_VECTOR(fusion.SV_9DOF_GBY_KALMAN.fbPl);
    #elif F_6DOF_GY_KALMAN
        PRINT_VECTOR(fusion.SV_6DOF_GY_KALMAN.fbPl);
    #endif
        printraw("\noffset error vector:");
    #if F_9DOF_GBY_KALMAN
        PRINT_VECTOR(fusion.SV_9DOF_GBY_KALMAN.fbErrPl);
    #elif F_6DOF_GY_KALMAN
        PRINT_VECTOR(fusion.SV_6DOF_GY_KALMAN.fbErrPl);
    #endif
        printraw("\n");
    }
    SaveGyroCalibrationToFlash(&fusion);
#endif

    // TODO: axis remap here
    // also code a HAL remap for each driver--it will normalize everything to XYZ and correct signs, this will be a
    // different axis remap for if users mount the IMU board in a different orientation

    // ACCEL: Iterate through all 12 positions and take an average accel measurment at each
    /* Note: positions 1-4 should be rotating 90° through roll axis, 5-8 through pitch axis, 9-12 through yaw axis
             positions 1, 5, and 9 are the same (level) position */
    for (u32 i = 0; i < MAX_ACCEL_CAL_ORIENTATIONS; i++) {
        char msg[64];
        sprintf(msg, "Calibration: please move to position #%lu/%d", i + 1, MAX_ACCEL_CAL_ORIENTATIONS);
        log_message(INFO, msg, 500, 200, true);
        if (runtime_is_fbw()) {
            char orientMsg[60] = {[0 ... 59] = ' '};
            sprintf(orientMsg, "Please move to position #%lu/%d", i + 1, MAX_ACCEL_CAL_ORIENTATIONS);
            display_string(orientMsg, (i + 1) * (33.0f / MAX_ACCEL_CAL_ORIENTATIONS) + 33);
        }
        // Wait for the sensor to move...
        while (!hasMoved) {
            aahrs.update();
            hasMoved = aahrs.rollRate > GYRO_STILL_VELOCITY || aahrs.pitchRate > GYRO_STILL_VELOCITY ||
                       aahrs.yawRate > GYRO_STILL_VELOCITY;
        }
        // ...and then for it to be still for 0.5s
        while (true) {
            aahrs.update();
            hasMoved = aahrs.rollRate > GYRO_STILL_VELOCITY || aahrs.pitchRate > GYRO_STILL_VELOCITY ||
                       aahrs.yawRate > GYRO_STILL_VELOCITY;
            if (!hasMoved) {
                wait = timestamp_in_ms(500);
                while (!hasMoved && !timestamp_reached(&wait)) {
                    aahrs.update();
                    hasMoved = aahrs.rollRate > GYRO_STILL_VELOCITY || aahrs.pitchRate > GYRO_STILL_VELOCITY ||
                               aahrs.yawRate > GYRO_STILL_VELOCITY;
                }
                if (timestamp_reached(&wait))
                    break;
            }
        }
        // Blink signify a position is being recorded
        log_message(INFO, "Calibration: recording position...", 250, 100, true);
        if (runtime_is_fbw())
            display_string("Recording position...", (i + 1) * (33.0f / MAX_ACCEL_CAL_ORIENTATIONS) + 33);
        // Set the current physical location of the sensor
        fusion.AccelBuffer.iStoreLocation = i;
        // Set the counter to the number of seconds to average over, and wait for the measurements to be taken (as well as for
        // the delay)
        fusion.AccelBuffer.iStoreCounter = (ACCEL_CAL_AVERAGING_SECS * FUSION_HZ);
        while (fusion.AccelBuffer.iStoreCounter > 0)
            aahrs.update();
    }
    printfbw(aahrs, "saving accelerometer calibration");
    if (shouldPrint.aahrs) {
        printraw("\noffset vector:");
        PRINT_VECTOR(fusion.AccelCal.fV);
        printraw("\ninverse gain matrix:");
        PRINT_MATRIX(fusion.AccelCal.finvW);
        printraw("\nforward rotation matrix (0):");
        PRINT_MATRIX(fusion.AccelCal.fR0);
        printraw("\n");
    }
    SaveAccelCalibrationToFlash(&fusion);

    // MAG: Collect mag measurements into buffer until a 10 element calibration is accepted by the algorithm
    log_message(INFO, "Calibration: please rotate on all axes", 1000, 200, true);
    for (u32 i = 0; i < MAX_MAG_ATTEMPTS; i++) {
        // Wait until the algorithm begins a calibration, and take measurements
        while (!fusion.MagCal.iCalInProgress) {
            printfbw(aahrs, "%d/%d measurements taken", fusion.MagBuffer.iMagBufferCount, MINMEASUREMENTS10CAL);
            if (runtime_is_fbw()) {
                char measureMsg[60] = {[0 ... 59] = ' '};
                sprintf(measureMsg, "%d/%d measurements taken", fusion.MagBuffer.iMagBufferCount, MINMEASUREMENTS10CAL);
                display_string(measureMsg, fusion.MagBuffer.iMagBufferCount * (33.0f / MAXMEASUREMENTS) + 66);
            }
            runtime_sleep_ms(1000, false);
        }
        // Calibration is being calculated, wait for it to finish
        printfbw(aahrs, "calibration in progress, please wait...");
        if (runtime_is_fbw())
            display_string("Calibration in progress, please wait",
                           fusion.MagBuffer.iMagBufferCount * (33.0f / MAXMEASUREMENTS) + 66);
        while (fusion.MagCal.iCalInProgress)
            aahrs.update();
        printfbw(aahrs, "fit error was %f (attempt %lu/%d)", fusion.MagCal.ftrFitErrorpc, i + 1, MAX_MAG_ATTEMPTS);
        // The optimal solver is the 10 element, so we're done if it was used for the most recent valid calibration
        if (fusion.MagCal.iValidMagCal >= 10)
            break;
    }
    if (fusion.MagCal.iValidMagCal < 10) {
        log_message(WARNING, "Mag calibration not optimal!", 1000, 0, false);
    }
    printfbw(aahrs, "[AAHRS] saving magnetometer calibration\n");
    if (shouldPrint.aahrs) {
        printraw("\nhard iron offset vector:");
        PRINT_VECTOR(fusion.MagCal.fV);
        printraw("\ninverse soft iron matrix:");
        PRINT_MATRIX(fusion.MagCal.finvW);
        printraw("geomagnetic field magnitude: %f", fusion.MagCal.fB);
        printraw("geomagnetic field magnitude^2: %f", fusion.MagCal.fBSq);
        printraw("calibration fit error: %f", fusion.MagCal.fFitErrorpc);
        printraw("calibration solver used: %ld", fusion.MagCal.iValidMagCal);
        printraw("\n");
    }
    SaveMagCalibrationToFlash(&fusion);
    // Flag AAHRS as calibrated, note the models at time of calibration, and save one last time
    calibration.aahrs[AAHRS_CALIBRATED] = true;
    calibration.aahrs[AAHRS_IMU_MODEL] = (IMUModel)config.sensors[SENSORS_IMU_MODEL];
    calibration.aahrs[AAHRS_BARO_MODEL] = (BaroModel)config.sensors[SENSORS_BARO_MODEL];
    config_save();
    log_clear(INFO);
    return true;
}

AAHRS aahrs = {
    .roll = INFINITY,
    .pitch = INFINITY,
    .yaw = INFINITY,
    .alt = -1,
    .init = aahrs_init,
    .deinit = aahrs_deinit,
    .update = aahrs_update,
    .calibrate = aahrs_calibrate,
    .isCalibrated = false // Will be set by aahrs_init() as applicable
};
