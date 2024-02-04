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
#include <stdlib.h>
#include "pico/platform.h"
#include "pico/time.h"
#include "pico/types.h"

#include "lib/fusion/acc.h"
#include "lib/fusion/calibration.h"
#include "lib/fusion/fconfig.h"
#include "lib/fusion/fusion.h"
#include "lib/fusion/mag.h"
#include "lib/fusion/status.h"

#include "lib/fusion/drivers/drivers.h"
#include "lib/fusion/drivers/ak09916.h"
#include "lib/fusion/drivers/bno055.h"
#include "lib/fusion/drivers/icm20948.h"

#include "sys/log.h"

#include "modes/aircraft.h"

#include "io/display.h"
#include "io/flash.h"
#include "io/platform.h"

#include "io/aahrs.h"

static SensorFusionGlobals fusion;
static StatusSubsystem status;
struct PhysicalSensor *sensors;
static uint numSensors = 0;
static uint rateDelay = 0; // Delay between fusion algorithm runs in microseconds
static absolute_time_t ranFusion; // Time of the last fusion algorithm run

bool aahrs_init() {
    // Check the state of any previous calibration
    aahrs.isCalibrated = (bool)flash.calibration[CALIBRATION_AAHRS_CALIBRATED];
    if (aahrs.isCalibrated && ((IMUModel)flash.calibration[CALIBRATION_AAHRS_IMU_MODEL] != (IMUModel)flash.sensors[SENSORS_IMU_MODEL] ||
                               (BaroModel)flash.calibration[CALIBRATION_AAHRS_BARO_MODEL] != (BaroModel)flash.sensors[SENSORS_BARO_MODEL])) {
        if (print.aahrs) printf("[AAHRS] calibration was performed on different models, recalibration is necessary!\n");
        // This ensures the system won't load any bad calibration into the fusion algorithms
        EraseFusionCalibration();
        aahrs.isCalibrated = false;
    }
    // Initialize (memory of) fusion systems
    if (print.aahrs) printf("[AAHRS] initialized ");
    initializeStatusSubsystem(&status);
    if (print.aahrs) printf("status, ");
    initSensorFusionGlobals(&fusion, &status);
    if (print.aahrs) printf("fusion memory defaults\n");
    // Install the sensors defined in the config
    if (print.fbw) printf("[AAHRS] installing ");
    switch ((IMUModel)flash.sensors[SENSORS_IMU_MODEL]) {
        case IMU_MODEL_BNO055:
            // Allocate memory for the sensor and install it
            if (print.fbw) printf("BNO055");
            numSensors++;
            sensors = reallocarray(sensors, numSensors, sizeof(struct PhysicalSensor));
            if (!sensors) return false;
            fusion.installSensor(&fusion, &sensors[numSensors - 1], BNO055_I2C_ADDR_LOW, 1, NULL, BNO055_init, BNO055_read);
            break;
        case IMU_MODEL_ICM20948:
            // The ICM20948 actually has an AK09916 inside it, but it has a different i2c address so we see it as a seperate device
            if (print.fbw) printf("AK09916");
            numSensors++;
            sensors = reallocarray(sensors, numSensors, sizeof(struct PhysicalSensor));
            if (!sensors) return false;
            fusion.installSensor(&fusion, &sensors[numSensors - 1], AK09916_I2C_ADDR, 1, NULL, AK09916_init, AK09916_read);
            if (print.fbw) printf(", ICM20948");
            numSensors++;
            sensors = reallocarray(sensors, numSensors, sizeof(struct PhysicalSensor));
            if (!sensors) return false;
            fusion.installSensor(&fusion, &sensors[numSensors - 1], ICM20948_I2C_ADDR_HIGH, 1, NULL, ICM20948_init, ICM20948_read);
            break;
        default:
            if (print.fbw) printf("\n[AAHRS] ERROR: unknown IMU model!\n");
            return false;
    }
    switch ((BaroModel)flash.sensors[SENSORS_BARO_MODEL]) {
        case BARO_MODEL_NONE:
            break;
        case BARO_MODEL_DPS310:
            if (print.fbw) printf(", DPS310");
            // Code incomplete
            break;
        default:
            if (print.fbw) printf("\n[AAHRS] ERROR: unknown baro model!\n");
            return false;
    }
    if (print.fbw) printf("\n[AAHRS] initializing core fusion engine\n");
    fusion.initializeFusionEngine(&fusion);
    // Ensure initialization has been good so far
    if (fusion.getStatus(&fusion) != NORMAL) return false;
    // Calculate the delay needed between fusion updates to obtain FUSION_HZ
    #if F_9DOF_GBY_KALMAN
        rateDelay = (uint)((1.0f / FUSION_HZ) * 1E6f - (F_9DOF_GBY_KALMAN_SYSTICK + F_CONDITION_SENSOR_READINGS_SYSTICK));
    #endif
    if (print.aahrs) printf("[AAHRS] to obtain update rate of %dHz, using delay of %dus\n", FUSION_HZ, rateDelay);
    return true;
}

void aahrs_deinit() {
    if (print.fbw) printf("[AAHRS] stopping!\n");
    aahrs.roll = INFINITY;
    aahrs.pitch = INFINITY;
    aahrs.yaw = INFINITY;
    aahrs.alt = -1;
}

void aahrs_update() {
    fusion.readSensors(&fusion, 0);
    // Only run fusion when we've waited for the rateDelay (essentially throttles algorithm)
    if (absolute_time_diff_us(ranFusion, get_absolute_time()) >= rateDelay) {
        fusion.conditionSensorReadings(&fusion);
        fusion.runFusion(&fusion);
        // Use a common pointer that will be usable no matter which fusion mode is running
        SV_ptr common;
        #if F_9DOF_GBY_KALMAN
            common = (SV_ptr)&fusion.SV_9DOF_GBY_KALMAN;
        #endif
        aahrs.roll = common->fPhi;
        aahrs.roll_rate = common->fOmega[CHX];
        aahrs.pitch = common->fThe;
        aahrs.pitch_rate = common->fOmega[CHY];
        aahrs.yaw = common->fPsi;
        aahrs.yaw_rate = common->fOmega[CHZ];
        // Alt will go here, not done yet
        ranFusion = get_absolute_time();
    }
}

bool aahrs_calibrate() {
    absolute_time_t wait;
    bool hasMoved = false;

    // GYRO: Wait for gyro to stabilize, calculate its offsets a number of times, then average them and save
    #if F_USING_GYRO && (F_9DOF_GBY_KALMAN || F_6DOF_GY_KALMAN)
        log_message(INFO, "Please hold still!", 1000, 200, true);
        if (platform_is_fbw()) {
            display_string("Please hold still!", 0);
        }
        // Wait for gyro (and user) to stabilize
        wait = make_timeout_time_ms(5000);
        while (!time_reached(wait)) aahrs.update();
        // Take GYRO_AVG_SAMPLES offsets
        float offsets[3];
        for (uint i = 0; i < GYRO_AVG_SAMPLES; i++) {
            if (platform_is_fbw()) {
                display_string("Please hold still!", (i + 1) * (33.0f / GYRO_AVG_SAMPLES));
            }
            // Reqest a reset from the algorithm so that the gyro offsets get calculated
            #if F_9DOF_GBY_KALMAN
                fusion.SV_9DOF_GBY_KALMAN.resetflag = true;
            #elif F_6DOF_GY_KALMAN
                fusion.SV_6DOF_GY_KALMAN.resetflag = true;
            #endif
            // Wait for the calculation and save
            wait = make_timeout_time_us(rateDelay);
            while (!time_reached(wait)) aahrs.update();
            #if F_9DOF_GBY_KALMAN
                for (uint j = 0; j < count_of(offsets); j++) {
                    offsets[j] += fusion.SV_9DOF_GBY_KALMAN.fbPl[j];
                }
            #elif F_6DOF_GY_KALMAN
                for (uint j = 0; j < count_of(offsets); j++) {
                    offsets[j] += fusion.SV_6DOF_GY_KALMAN.fbPl[j];
                }
            #endif
        }
        // Average the offsets and save
        for (uint i = 0; i < count_of(offsets); i++) {
            #if F_9DOF_GBY_KALMAN
                fusion.SV_9DOF_GBY_KALMAN.fbPl[i] = offsets[i] / GYRO_AVG_SAMPLES;
            #elif F_6DOF_GY_KALMAN
                fusion.SV_6DOF_GY_KALMAN.fbPl[i] = offsets[i] / GYRO_AVG_SAMPLES;
            #endif
        }
        if (print.fbw) printf("[AAHRS] saving gyro calibration\n");
        if (print.aahrs) {
            printf("\noffset vector:\n");
            #if F_9DOF_GBY_KALMAN
                PRINT_VECTOR(fusion.SV_9DOF_GBY_KALMAN.fbPl);
            #elif F_6DOF_GY_KALMAN
                PRINT_VECTOR(fusion.SV_6DOF_GY_KALMAN.fbPl);
            #endif
            printf("\noffset error vector:\n");
            #if F_9DOF_GBY_KALMAN
                PRINT_VECTOR(fusion.SV_9DOF_GBY_KALMAN.fbErrPl);
            #elif F_6DOF_GY_KALMAN
                PRINT_VECTOR(fusion.SV_6DOF_GY_KALMAN.fbErrPl);
            #endif
            printf("\n");
        }
        SaveGyroCalibrationToFlash(&fusion);
    #endif

    // TODO: axis remap here
    // also code a HAL remap for each driver--it will normalize everything to XYZ and correct signs, this will be a
    // different axis remap for if users mount the IMU board in a different orientation

    // ACCEL: Iterate through all 12 positions and take an average accel measurment at each
    /* Note: positions 1-4 should be rotating 90° through roll axis, 5-8 through pitch axis, 9-12 through yaw axis
             positions 1, 5, and 9 are the same (level) position */
    for (uint i = 0; i < MAX_ACCEL_CAL_ORIENTATIONS; i++) {
        char msg[64];
        sprintf(msg, "Calibration: please move to position #%d/%d", i + 1, MAX_ACCEL_CAL_ORIENTATIONS);
        log_message(INFO, msg, 500, 200, true);
        if (platform_is_fbw()) {
            char orientMsg[60] = { [0 ... 59] = ' '};
            sprintf(orientMsg, "Please move to position #%d/%d", i + 1, MAX_ACCEL_CAL_ORIENTATIONS);
            display_string(orientMsg, (i + 1) * (33.0f / MAX_ACCEL_CAL_ORIENTATIONS) + 33);
        }
        // Wait for the sensor to move...
        while (!hasMoved) {
            aahrs.update();
            hasMoved = aahrs.roll_rate > GYRO_STILL_VELOCITY || aahrs.pitch_rate > GYRO_STILL_VELOCITY || aahrs.yaw_rate > GYRO_STILL_VELOCITY;
        }
        // ...and then for it to be still for 0.5s
        while (true) {
            aahrs.update();
            hasMoved = aahrs.roll_rate > GYRO_STILL_VELOCITY || aahrs.pitch_rate > GYRO_STILL_VELOCITY || aahrs.yaw_rate > GYRO_STILL_VELOCITY;
            if (!hasMoved) {
                wait = make_timeout_time_ms(500);
                while (!hasMoved && !time_reached(wait)) {
                    aahrs.update();
                    hasMoved = aahrs.roll_rate > GYRO_STILL_VELOCITY || aahrs.pitch_rate > GYRO_STILL_VELOCITY || aahrs.yaw_rate > GYRO_STILL_VELOCITY;
                }
                if (time_reached(wait)) break;
            }
        }
        // Blink signify a position is being recorded
        log_message(INFO, "Calibration: recording position...", 250, 100, true);
        if (platform_is_fbw()) {
            display_string("Recording position...", (i + 1) * (33.0f / MAX_ACCEL_CAL_ORIENTATIONS) + 33);
        }
        // Set the current physical location of the sensor
        fusion.AccelBuffer.iStoreLocation = i;
        // Set the counter to the number of seconds to average over, and wait for the measurements to be taken (as well as for the delay)
        fusion.AccelBuffer.iStoreCounter = (ACCEL_CAL_AVERAGING_SECS * FUSION_HZ);
        while (fusion.AccelBuffer.iStoreCounter > 0) aahrs.update();
    }
    if (print.fbw) printf("[AAHRS] saving accelerometer calibration\n");
    if (print.aahrs) {
        printf("\noffset vector:\n");
        PRINT_VECTOR(fusion.AccelCal.fV);
        printf("\ninverse gain matrix:\n");
        PRINT_MATRIX(fusion.AccelCal.finvW);
        printf("\nforward rotation matrix (0):\n");
        PRINT_MATRIX(fusion.AccelCal.fR0);
        printf("\n");
    }
    SaveAccelCalibrationToFlash(&fusion);

    // MAG: Collect mag measurements into buffer until a 10 element calibration is accepted by the algorithm
    log_message(INFO, "Calibration: please rotate on all axes", 1000, 200, true);
    for (uint i = 0; i < MAX_MAG_ATTEMPTS; i++) {
        // Wait until the algorithm begins a calibration, and take measurements
        while (!fusion.MagCal.iCalInProgress) {
            if (print.fbw) printf("[AAHRS] %d/%d measurements taken\n", fusion.MagBuffer.iMagBufferCount, MINMEASUREMENTS10CAL);
            if (platform_is_fbw()) {
                char measureMsg[60] = { [0 ... 59] = ' '};
                sprintf(measureMsg, "%d/%d measurements taken", fusion.MagBuffer.iMagBufferCount, MINMEASUREMENTS10CAL);
                display_string(measureMsg, fusion.MagBuffer.iMagBufferCount * (33.0f / MAXMEASUREMENTS) + 66);
            }
            platform_sleep_ms(1000, false);
        }
        // Calibration is being calculated, wait for it to finish
        if (print.fbw) printf("[AAHRS] calibration in progress, please wait...\n");
        if (platform_is_fbw()) {
            display_string("Calibration in progress, please wait", fusion.MagBuffer.iMagBufferCount * (33.0f / MAXMEASUREMENTS) + 66);
        }
        while (fusion.MagCal.iCalInProgress) aahrs.update();
        printf("[AAHRS] fit error was %f (attempt %d/%d)\n", fusion.MagCal.ftrFitErrorpc, i + 1, MAX_MAG_ATTEMPTS);
        // The optimal solver is the 10 element, so we're done if it was used for the most recent valid calibration
        if (fusion.MagCal.iValidMagCal >= 10) break;
    }
    if (fusion.MagCal.iValidMagCal < 10) {
        log_message(WARNING, "Mag calibration not optimal!", 1000, 0, false);
    }
    if (print.fbw) printf("[AAHRS] saving magnetometer calibration\n");
    if (print.aahrs) {
        printf("\nhard iron offset vector:\n");
        PRINT_VECTOR(fusion.MagCal.fV);
        printf("\ninverse soft iron matrix:\n");
        PRINT_MATRIX(fusion.MagCal.finvW);
        printf("\ngeomagnetic field magnitude:\n%f\n", fusion.MagCal.fB);
        printf("\ngeomagnetic field magnitude^2:\n%f\n", fusion.MagCal.fBSq);
        printf("\ncalibration fit error\n%f\n", fusion.MagCal.fFitErrorpc);
        printf("\ncalibration solver used\n%d\n", fusion.MagCal.iValidMagCal);
        printf("\n");
    }
    SaveMagCalibrationToFlash(&fusion);
    // Flag AAHRS as calibrated, note the models at time of calibration, and save one last time
    flash.calibration[CALIBRATION_AAHRS_CALIBRATED] = true;
    flash.calibration[CALIBRATION_AAHRS_IMU_MODEL] = (IMUModel)flash.sensors[SENSORS_IMU_MODEL];
    flash.calibration[CALIBRATION_AAHRS_BARO_MODEL] = (BaroModel)flash.sensors[SENSORS_BARO_MODEL];
    flash_save();
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
