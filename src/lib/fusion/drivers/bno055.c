/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright (C) 2015 - 2016 Bosch Sensortec GmbH
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

#include "pico/types.h"

#include "bno055.h"

/* 
   BNO055 
   Datasheet: https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bno055-ds000.pdf
*/

const registerReadlist_t BNO055_WHO_AM_I_READ[] = {
    { .readFrom = BNO055_CHIP_ID, .numBytes = 1 }, __END_READ_DATA__
};

registerReadlist_t BNO055_ACCEL_DATA_READ[] = {
    { .readFrom = BNO055_ACCEL_DATA_X_LSB, .numBytes = 6 }, __END_READ_DATA__
};

registerReadlist_t BNO055_MAG_DATA_READ[] = {
    { .readFrom = BNO055_MAG_DATA_X_LSB, .numBytes = 6 }, __END_READ_DATA__
};

registerReadlist_t BNO055_GYRO_DATA_READ[] = {
    { .readFrom = BNO055_GYRO_DATA_X_LSB, .numBytes = 6 }, __END_READ_DATA__
};

// Each entry in a RegisterWriteList is composed of: register address, value to write, bit-mask to apply to write (0 enables)
const registerwritelist_t BNO055_Initialization[] = {
    { BNO055_SYS_TRIGGER, 0x00, 0x00 }, // Use internal oscillator
    { BNO055_PWR_MODE, BNO055_POWER_MODE_NORMAL, 0x00 },
    { BNO055_UNIT_SEL, 0x01, 0x00 }, // Select units (Windows orientation, °C, °, °/s, mg)
    { BNO055_OPR_MODE, BNO055_OPERATION_MODE_AMG, 0x00 },
    __END_WRITE_DATA__
};

/* All sensor drivers and initialization functions have a similar prototype
sensor = pointer to linked list element used by the sensor fusion subsystem to specify required sensors
sfg = pointer to top level data structure for sensor fusion */

int8_t BNO055_init_accel(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg) {
    // TODO: do we need to modify sensor bandwidths / ODRs? (also for other sensor inits)
    return SENSOR_ERROR_NONE;
}

// We use mG (milli-G) units of acceleration which are 1:1 (datasheet pg. 33) and there are 1000 mG in one G, thus 1000 CPG
#define BNO055_COUNTS_PER_G 1000

int8_t BNO055_init_mag(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg) {
    return SENSOR_ERROR_NONE;
}

#define BNO055_COUNTS_PER_UT 16 // (datasheet pg. 34)

int8_t BNO055_init_gyro(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg) {
    return SENSOR_ERROR_NONE;
}

#define BNO055_COUNTS_PER_DPS 16 // (datasheet pg. 35)

int8_t BNO055_init(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg) {
    int32_t status;
    uint8_t reg;
    // Check that the sensor comms are okay and that it's a BNO055
    status = driver_read_register(&sensor->deviceInfo, sensor->addr, BNO055_WHO_AM_I_READ[0].readFrom, BNO055_WHO_AM_I_READ[0].numBytes, &reg);
    if (status != SENSOR_ERROR_NONE) return status;
    if (reg != BNO055_CHIP_WHO_AM_I) {
        return SENSOR_ERROR_INIT;
    }
    // Set up sensor data for fusion algorithms later
    #if F_USING_ACCEL
        sfg->Accel.iWhoAmI = reg;
        sfg->Accel.iCountsPerg = BNO055_COUNTS_PER_G;
        sfg->Accel.fCountsPerg = (float)BNO055_COUNTS_PER_G;
        sfg->Accel.fgPerCount = 1.0f/BNO055_COUNTS_PER_G;
    #endif
    #if F_USING_MAG
        sfg->Mag.iWhoAmI = reg;
        sfg->Mag.iCountsPeruT = BNO055_COUNTS_PER_UT;
        sfg->Mag.fCountsPeruT = (float)BNO055_COUNTS_PER_UT;
        sfg->Mag.fuTPerCount = 1.0f/BNO055_COUNTS_PER_UT;
    #endif
    #if F_USING_GYRO
        sfg->Gyro.iWhoAmI = reg;
        sfg->Gyro.iCountsPerDegPerSec = BNO055_COUNTS_PER_DPS;
        sfg->Gyro.fDegPerSecPerCount = 1.0f/BNO055_COUNTS_PER_DPS;
    #endif

    // Configure sensor
    status = driver_write_list(&sensor->deviceInfo, sensor->addr, BNO055_Initialization);
    sensor->isInitialized = F_USING_ACCEL | F_USING_MAG;
    #if F_USING_ACCEL
        sfg->Accel.isEnabled = true;
    #endif
    #if F_USING_MAG
        sfg->Mag.isEnabled = true;
    #endif
    #if F_USING_GYRO
        sfg->Gyro.isEnabled = true;
    #endif

    return status;
}

#if F_USING_ACCEL
int8_t BNO055_read_accel(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg) {
    if (!(sensor->isInitialized & F_USING_ACCEL)) return SENSOR_ERROR_INIT;
    unsigned char buf[ACCEL_FIFO_SIZE * 6];
    int32_t status = driver_read(&sensor->deviceInfo, sensor->addr, BNO055_ACCEL_DATA_READ, buf);
    if (status == SENSOR_ERROR_NONE) {
        int16_t sample[3];
        // Shift LSB and MSB together
        for (uint i = 0; i < count_of(sample); i++) {
            sample[i] = (int16_t)((buf[i * 2 + 1] << 8) | buf[i * 2]);
        }
        // Normalize data and add to FIFO
        conditionSample(sample);
        addToFifo((union FifoSensor*) &(sfg->Accel), ACCEL_FIFO_SIZE, sample);
    } else return status;
}
#endif

#if F_USING_MAG
int8_t BNO055_read_mag(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg) {
    if (!(sensor->isInitialized & F_USING_MAG)) return SENSOR_ERROR_INIT;
    unsigned char buf[MAG_FIFO_SIZE * 6];
    int32_t status = driver_read(&sensor->deviceInfo, sensor->addr, BNO055_MAG_DATA_READ, buf);
    if (status == SENSOR_ERROR_NONE) {
        int16_t sample[3];
        for (uint i = 0; i < count_of(sample); i++) {
            sample[i] = (int16_t)((buf[i * 2 + 1] << 8) | buf[i * 2]);
        }
        conditionSample(sample);
        addToFifo((union FifoSensor*) &(sfg->Mag), MAG_FIFO_SIZE, sample);
    } else return status;
}
#endif

#if F_USING_GYRO
int8_t BNO055_read_gyro(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg) {
    if (!(sensor->isInitialized & F_USING_GYRO)) return SENSOR_ERROR_INIT;
    unsigned char buf[GYRO_FIFO_SIZE * 6];
    int32_t status = driver_read(&sensor->deviceInfo, sensor->addr, BNO055_GYRO_DATA_READ, buf);
    if (status == SENSOR_ERROR_NONE) {
        int16_t sample[3];
        for (uint i = 0; i < count_of(sample); i++) {
            sample[i] = (int16_t)((buf[i * 2 + 1] << 8) | buf[i * 2]);
        }
        conditionSample(sample);
        addToFifo((union FifoSensor*) &(sfg->Gyro), GYRO_FIFO_SIZE, sample);
    } else return status;
}
#endif

int8_t BNO055_read(struct PhysicalSensor *sensor, SensorFusionGlobals *sfg) {
    int8_t status_accel = 0, status_mag = 0, status_gyro = 0;
    #if F_USING_ACCEL
        status_accel = BNO055_read_accel(sensor, sfg);
    #endif
    #if F_USING_MAG
        status_mag = BNO055_read_mag(sensor, sfg);
    #endif
    #if F_USING_GYRO
        status_gyro = BNO055_read_gyro(sensor, sfg);
    #endif
    return (status_accel + status_mag + status_gyro);
}
