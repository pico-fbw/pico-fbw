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

#include <string.h>
#include "platform/helpers.h"

#include "drivers/drivers.h"
#include "drivers/icm20948.h"

#include "sys/print.h"

#include "fusion.h"

GyroscopeDetails gyroscopes[] = {
    {"ICM20948",
     {0x69, 0x68},
     {.create = icm20948_gyro_create,
      .read = icm20948_gyro_read,
      .get_odr = icm20948_gyro_get_odr,
      .set_odr = icm20948_gyro_set_odr,
      .get_scale = icm20948_gyro_get_scale,
      .set_scale = icm20948_gyro_set_scale},
     icm20948_gyro_detect,
     icm20948_state_create,
     icm20948_state_destroy},
};

bool fusion_gyroscope_find(IMU *imu, const GyroscopeOptions *opts) {
    if (!imu)
        return false;

    for (u32 i = 0; i < count_of(gyroscopes); i++) {
        imu->gyro = &(gyroscopes[i].device);
        bool detected = false;
        if (!imu->state && gyroscopes[i].create_state) {
            imu->state = gyroscopes[i].create_state();
            if (!imu->state) {
                printfbw(aahrs, "ERROR: could not create user data for gyroscope \"%s\"", gyroscopes[i].name);
                return false;
            }
        }
        if (gyroscopes[i].detect) {
            for (u32 a = 0; a < count_of(gyroscopes[i].addr); a++) {
                byte addr = gyroscopes[i].addr[a];
                if (addr == NOADDR)
                    continue;
                printfbw(aahrs, "Scanning for gyroscope \"%s\" at I2C 0x%02x", gyroscopes[i].name, addr);
                if (gyroscopes[i].detect(addr, imu->state)) {
                    printfbw(aahrs, "Detected gyroscope \"%s\" at I2C 0x%02x", gyroscopes[i].name, addr);
                    imu->gyro->addr = addr;
                    detected = true;
                    break;
                }
            }
        }
        if (detected) {
            imu->gyro->opts = *opts;
            if (imu->gyro->create) {
                if (!imu->gyro->create(imu->gyro, imu->state)) {
                    printfbw(aahrs, "ERROR: could not create gyroscope \"%s\" at I2C 0x%02x", gyroscopes[i].name,
                             imu->gyro->addr);
                    if (imu->gyro->destroy)
                        imu->gyro->destroy(imu->gyro, imu->state);
                    if (imu->state)
                        free(imu->state);
                    imu->state = NULL;
                    return false;
                } else {
                    printfbw(aahrs, "Successfully created gyroscope \"%s\" at I2C 0x%02x", gyroscopes[i].name, imu->gyro->addr);
                }
            }
            if (imu->gyro->set_scale)
                imu->gyro->set_scale(imu->gyro, imu->state, opts->scale);
            if (imu->gyro->set_odr)
                imu->gyro->set_odr(imu->gyro, imu->state, opts->odr);
            imu->gyro->orientation[0] = 1.f;
            imu->gyro->orientation[1] = 0.f;
            imu->gyro->orientation[2] = 0.f;
            imu->gyro->orientation[3] = 0.f;
            imu->gyro->orientation[4] = 1.f;
            imu->gyro->orientation[5] = 0.f;
            imu->gyro->orientation[6] = 0.f;
            imu->gyro->orientation[7] = 0.f;
            imu->gyro->orientation[8] = 1.f;
            return true;
        } else {
            // Nothing detected, clean up state if created
            if (imu->state && gyroscopes[i].create_state && gyroscopes[i].destroy_state)
                imu->state = gyroscopes[i].destroy_state(imu->state);
            memset(imu->gyro, 0, sizeof(Gyroscope));
        }
    }
    // Nothing was detected
    imu->gyro = NULL;
    return false;
}

bool fusion_gyroscope_get(IMU *imu, f32 *x, f32 *y, f32 *z) {
    if (!imu->gyro || !imu->gyro->read)
        return false;
    if (!imu->gyro->read(imu->gyro, imu->state)) {
        printfbw(aahrs, "ERROR: could not read from gyroscope");
        return false;
    }

    // LOG(LL_DEBUG, ("Raw: gx=%d gy=%d gz=%d", imu->gyro->gx, imu->gyro->gy, imu->gyro->gz));
    if (x) {
        *x = (imu->gyro->scale * (imu->gyro->gx * imu->gyro->orientation[0] + imu->gyro->gy * imu->gyro->orientation[1] +
                                  imu->gyro->gz * imu->gyro->orientation[2])) +
             imu->gyro->offset_gx;
    }
    if (y) {
        *y = (imu->gyro->scale * (imu->gyro->gx * imu->gyro->orientation[3] + imu->gyro->gy * imu->gyro->orientation[4] +
                                  imu->gyro->gz * imu->gyro->orientation[5])) +
             imu->gyro->offset_gy;
    }
    if (z) {
        *z = (imu->gyro->scale * (imu->gyro->gx * imu->gyro->orientation[6] + imu->gyro->gy * imu->gyro->orientation[7] +
                                  imu->gyro->gz * imu->gyro->orientation[8])) +
             imu->gyro->offset_gz;
    }
    return true;
}

bool fusion_gyroscope_set_offset(IMU *imu, f32 x, f32 y, f32 z) {
    if (!imu || !imu->gyro)
        return false;

    imu->gyro->offset_gx = x;
    imu->gyro->offset_gy = y;
    imu->gyro->offset_gz = z;
    return true;
}

bool fusion_gyroscope_get_offset(IMU *imu, f32 *x, f32 *y, f32 *z) {
    if (!imu || !imu->gyro)
        return false;

    if (x)
        *x = imu->gyro->offset_gx;
    if (y)
        *y = imu->gyro->offset_gy;
    if (z)
        *z = imu->gyro->offset_gz;
    return true;
}

bool fusion_gyroscope_get_orientation(IMU *imu, f32 v[9]) {
    if (!imu || !imu->gyro || !v)
        return false;

    memcpy(v, imu->gyro->orientation, sizeof(f32) * 9);
    return true;
}

bool fusion_gyroscope_set_orientation(IMU *imu, f32 v[9]) {
    if (!imu || !imu->gyro || !v)
        return false;

    memcpy(imu->gyro->orientation, v, sizeof(f32) * 9);
    return true;
}

bool fusion_gyroscope_get_scale(IMU *imu, f32 *scale) {
    if (!imu || !imu->gyro || !imu->gyro->get_scale || !scale)
        return false;

    return imu->gyro->get_scale(imu->gyro, imu->state, scale);
}

bool fusion_gyroscope_set_scale(IMU *imu, f32 scale) {
    if (!imu || !imu->gyro || !imu->gyro->set_scale)
        return false;

    return imu->gyro->set_scale(imu->gyro, imu->state, scale);
}

bool fusion_gyroscope_get_odr(IMU *imu, f32 *hertz) {
    if (!imu || !imu->gyro || !imu->gyro->get_odr || !hertz)
        return false;

    return imu->gyro->get_odr(imu->gyro, imu->state, hertz);
}

bool fusion_gyroscope_set_odr(IMU *imu, f32 hertz) {
    if (!imu || !imu->gyro || !imu->gyro->set_odr)
        return false;

    return imu->gyro->set_odr(imu->gyro, imu->state, hertz);
}
