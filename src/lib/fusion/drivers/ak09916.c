/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * Copyright (c) 2020 Bjarne Hansen
 * All rights reserved.
 *
 * This file utilizes code under the BSD-3-Clause License. See "LICENSE" for details.
*/

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include "pico/platform.h"
#include "pico/types.h"

#include "../../../io/aahrs.h"
#include "../../../io/flash.h"

#include "ak09916.h"

/* 
   AK09916
   Datasheet: https://www.y-ic.es/datasheet/78/SMDSW.020-2OZ.pdf (also in pages 77-80 of the ICM20948 datasheet)
   Note: The BNO055 was the first sensor driver written so most comments are in that file. Only differing operations are documented here.

   The AK09916 is built in to the ICM20948, but due to the way the ICM20948 is designed, the AK09916 shows up to us as a seperate device.
   Thus, there is a seperate driver for each sensor, but both will be installed when the ICM20948 is selected. Confusing, right?
*/

const registerReadList_t AK09916_DEVICE_ID_READ[] = {
    { .readFrom = AK09916_DEVICE_ID, .numBytes = 1 }, __END_READ_DATA__
};

const registerReadList_t AK09916_MAG_DATA_READ[] = {
    { .readFrom = AK09916_MAG_DATA_X_L, .numBytes = 6 },
    { .readFrom = AK09916_STATUS_2, .numBytes = 1 }, // Required as per datasheet
    __END_READ_DATA__
};

const registerWriteList_t AK09916_CONFIGURE[] = {
    { AK09916_CONTROL_3, 0x01, 0x00 }, // Reset
    { AK09916_CONTROL_2, 0x06, 0x00 }, // 50Hz ODR
    __END_WRITE_DATA__
};

#define AK09916_COUNTS_PER_UT 6.666667f // 0.15μT/LSB (datasheet pg. 6), so that works out to 6.66... LSB/uT
#define AK09916_UT_PER_COUNT 0.15f

int8_t AK09916_init(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg) {
    int32_t status;
    uint8_t reg;
    if (print.aahrs) printf("[AK09916] initializing...");
    for (uint i = 0; i < DRIVER_INIT_ATTEMPTS; i++) {
        if (print.aahrs) printf("attempt %d ", i);
        status = driver_read_register(&sensor->deviceInfo, sensor->addr, AK09916_DEVICE_ID_READ[0].readFrom, AK09916_DEVICE_ID_READ[0].numBytes, &reg);
        if (status == SENSOR_ERROR_NONE && reg == AK09916_DEVICE_ID_EXPECTED) break;
    }
    if (print.aahrs) printf("\n");
    if (status != SENSOR_ERROR_NONE) {
        if (print.fbw) printf("[AK09916] ERROR: address not acknowledged! (no/wrong device present?)\n");
        return status;
    }
    if (reg != AK09916_DEVICE_ID_EXPECTED) {
        if (print.fbw) printf("[AK09916] ERROR: could not verify chip!\n");
        return SENSOR_ERROR_INIT;
    }
    #if F_USING_MAG
        sfg->Mag.iWhoAmI = reg;
        sfg->Mag.iCountsPeruT = (int16_t)(AK09916_COUNTS_PER_UT + 0.5f);
        sfg->Mag.fCountsPeruT = AK09916_COUNTS_PER_UT;
        sfg->Mag.fuTPerCount = AK09916_UT_PER_COUNT; // Since counts per μT is imprecisely derived from μT per count, we use the real value here
    #endif

    if (print.aahrs) printf("[AK09916] configuring...\n");
    status = driver_write_list(&sensor->deviceInfo, sensor->addr, AK09916_CONFIGURE);
    #if F_USING_MAG
        sfg->Mag.isEnabled = true;
    #endif
    if (print.aahrs) printf("[AK09916] sensor ready!\n");
    sensor->isInitialized = F_USING_MAG;
    return status;
}

#if F_USING_MAG
int8_t AK09916_read_mag(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg) {
    if (!(sensor->isInitialized & F_USING_MAG)) return SENSOR_ERROR_INIT;
    uint8_t buf[6];
    int32_t status = driver_read(&sensor->deviceInfo, sensor->addr, AK09916_MAG_DATA_READ, buf);
    if (status == SENSOR_ERROR_NONE) {
        int16_t sample[3];
        for (uint i = 0; i < count_of(sample); i++) {
            sample[i] = (buf[i * 2 + 1] << 8) | buf[i * 2];
        }
        conditionSample(sample);
        addToFifo((union FifoSensor*) &(sfg->Mag), MAG_FIFO_SIZE, sample);
    }
    return status;
}
#endif

int8_t AK09916_read(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg) {
    #if F_USING_MAG
        return AK09916_read_mag(sensor, sfg);
    #else
        return 0;
    #endif
}
