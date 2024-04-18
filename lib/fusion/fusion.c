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

#include <math.h>
#include <stdlib.h>

#include "drivers/drivers.h"

#include "sys/print.h"

#include "fusion.h"

IMU *fusion_imu_create(void) {
    IMU *imu;
    imu = calloc(1, sizeof(IMU));
    if (!imu)
        return NULL;
    return imu;
}

void fusion_imu_destroy(IMU **imu) {
    if (!*imu)
        return;
    if ((*imu)->state)
        free((*imu)->state);
    free(*imu);
    *imu = NULL;
}

bool fusion_accelerometer_present(IMU *imu) {
    if (!imu)
        return false;
    return imu->acc != NULL;
}

bool fusion_gyroscope_present(IMU *imu) {
    if (!imu)
        return false;
    return imu->gyro != NULL;
}

bool fusion_magnetometer_present(IMU *imu) {
    if (!imu)
        return false;
    return imu->mag != NULL;
}
