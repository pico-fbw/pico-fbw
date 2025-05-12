/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "drivers.h"

// https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmi270-ds000.pdf

#define BMI270_ADDR_LOW 0x68
#define BMI270_ADDR_HIGH 0x69

#define BMI270_REG_ID 0x00

#define BMI270_DEVICE_ID 0x24

bool bmi270_exists(FusionDriver *self) {
    return check_devid(self, BMI270_ADDR_LOW, BMI270_ADDR_HIGH, BMI270_REG_ID, BMI270_DEVICE_ID);
}

bool bmi270_init(FusionDriver *self) {
    return true;
}

bool bmi270_read(FusionDriver *self, f32 data[]) {
    return true;
}

void bmi270_destroy(FusionDriver *self) {
    return;
}

const FusionDevice bmi270 = {
    .acc = &((FusionDriver){
        .exists = bmi270_exists,
        .init = bmi270_init,
        .read = bmi270_read,
        .destroy = bmi270_destroy,
    }),
    .gyro = &((FusionDriver){
        .exists = bmi270_exists,
        .init = bmi270_init,
        .read = bmi270_read,
        .destroy = bmi270_destroy,
    }),
    .name = "BMI270",
};
