/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "drivers.h"

// https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmm350-ds001.pdf

#define BMM350_ADDR_LOW 0x14
#define BMM350_ADDR_HIGH 0x15

#define BMM350_REG_ID 0x00

#define BMM350_DEVICE_ID 0x33

bool bmm350_exists(FusionDriver *self) {
    return check_devid(self, BMM350_ADDR_LOW, BMM350_ADDR_HIGH, BMM350_REG_ID, BMM350_DEVICE_ID);
}

bool bmm350_init(FusionDriver *self) {
    return true;
}

bool bmm350_read(FusionDriver *self, f32 data[]) {
    return true;
}

void bmm350_destroy(FusionDriver *self) {
    return;
}

FusionDriver bmm350_driver = {
    .exists = bmm350_exists,
    .init = bmm350_init,
    .read = bmm350_read,
    .destroy = bmm350_destroy,
};
const FusionDevice bmm350 = {
    .mag = &bmm350_driver,
    .name = "BMM350",
};
