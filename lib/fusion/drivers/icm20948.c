/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "drivers.h"

// https://invensense.tdk.com/wp-content/uploads/2016/06/DS-000189-ICM-20948-v1.3.pdf

#define ICM20948_ADDR_LOW 0x68
#define ICM20948_ADDR_HIGH 0x69
#define AK09916_ADDR 0x0C

#define ICM20948_REG_WHO_AM_I 0x00
#define AK09916_REG_WIA2 0x01

#define ICM20948_DEVICE_ID 0xEA
#define AK09916_DEVICE_ID 0x09

bool icm20948_exists(FusionDriver *self) {
    return check_devid(self, ICM20948_ADDR_LOW, ICM20948_ADDR_HIGH, ICM20948_REG_WHO_AM_I, ICM20948_DEVICE_ID);
}

bool ak09916_exists(FusionDriver *self) {
    return check_devid(self, AK09916_ADDR, 0, AK09916_REG_WIA2, AK09916_DEVICE_ID);
}

bool icm20948_acc_init(FusionDriver *self) {
}

bool icm20948_gyro_init(FusionDriver *self) {
}

bool ak09916_init(FusionDriver *self) {
}

bool icm20948_acc_read(FusionDriver *self, f32 data[]) {
}

bool icm20948_gyro_read(FusionDriver *self, f32 data[]) {
}

bool ak09916_read(FusionDriver *self, f32 data[]) {
}

const FusionDevice icm20948 = {
    .acc = &((FusionDriver){
        .exists = icm20948_exists,
        .init = icm20948_acc_init,
        .read = icm20948_acc_read,
    }),
    .gyro = &((FusionDriver){
        .exists = icm20948_exists,
        .init = icm20948_gyro_init,
        .read = icm20948_gyro_read,
    }),
    .mag = &((FusionDriver){
        .exists = ak09916_exists,
        .init = ak09916_init,
        .read = ak09916_read,
    }),
    .name = "ICM-20948",
};
