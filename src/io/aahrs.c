/*
 * Copyright (c) 2020-2021, Bjarne Hansen
 * All rights reserved.
 *
 * This file utilizes code under the BSD-3-Clause License. See "LICENSE" for details.
*/

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <math.h>
#include <stdlib.h>
#include "pico/time.h"
#include "pico/types.h"

#include "../lib/fusion/fconfig.h"
#include "../lib/fusion/fusion.h"
#include "../lib/fusion/status.h"

#include "../lib/fusion/drivers/drivers.h"
#include "../lib/fusion/drivers/bno055.h"
#include "../lib/fusion/drivers/icm20948.h"

#include "../sys/log.h"

#include "../modes/modes.h"

#include "flash.h"

#include "aahrs.h"

static SensorFusionGlobals fusion;
static StatusSubsystem status;
static struct PhysicalSensor *sensors;
static uint rateDelay = 0; // Delay between fusion algorithm runs in microseconds
static absolute_time_t ranFusion; // Time of the last fusion algorithm run

bool aahrs_init() {
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
            if (print.fbw) printf("BNO055, ");
            sensors = malloc(sizeof(struct PhysicalSensor));
            fusion.installSensor(&fusion, &sensors[0], BNO055_I2C_ADDR_LOW, 1, NULL, BNO055_init, BNO055_read);
            break;
        case IMU_MODEL_ICM20948:
            // Code incomplete
            break;
        default:
            if (print.fbw) printf("\n[AAHRS] ERROR: unknown IMU model!\n");
            return false;
    }
    switch ((BaroModel)flash.sensors[SENSORS_BARO_MODEL]) {
        case BARO_MODEL_NONE:
            break;
        case BARO_MODEL_DPS310:
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
    rateDelay = (uint)((1.0f / FUSION_HZ) * 1000000 - F_9DOF_GBY_KALMAN_SYSTICK);
    if (print.aahrs) printf("[AAHRS] to obtain update rate of %dHz, using delay of %dus\n", FUSION_HZ, rateDelay);
    
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
    fusion.conditionSensorReadings(&fusion);
    // Only run fusion when we've waited for the rateDelay (essentially throttles algorithm)
    if (absolute_time_diff_us(ranFusion, get_absolute_time()) >= rateDelay) {
        fusion.runFusion(&fusion);
        aahrs.roll = fusion.SV_9DOF_GBY_KALMAN.fPhiPl;
        aahrs.pitch = fusion.SV_9DOF_GBY_KALMAN.fThePl;
        aahrs.yaw = fusion.SV_9DOF_GBY_KALMAN.fPsiPl;
        // Alt here, not done yet
        ranFusion = get_absolute_time();
    }
}

bool aahrs_calibrate() {
    // TODO: do we have to invoke calibration or is it automatic?
    // at least give a procedure for the user here, because I know for a fact you have to move the sensor around a bit
}

AAHRS aahrs = {
    .roll = INFINITY,
    .pitch = INFINITY,
    .yaw = INFINITY,
    .alt = -1,
    .init = aahrs_init,
    .deinit = aahrs_deinit,
    .update = aahrs_update,
    .calibrate = aahrs_calibrate
};
