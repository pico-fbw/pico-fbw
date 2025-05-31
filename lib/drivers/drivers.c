/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "platform/helpers.h"
#include "platform/i2c.h"

#include "sys/print.h"

#include "drivers.h"

// Master device list; devices are defined in their respective driver source files
const FusionDevice *fusionDevices[] = {
    &bme280,
    &bmi270,
    &bmm350,
};
const u32 numFusionDevices = count_of(fusionDevices);

bool check_devid(FusionDriver *driver, byte addr, byte alt_addr, byte reg, byte expected) {
    if (i2c_read_byte(ASDA, ASCL, addr, reg) == expected) {
        driver->addr = addr;
        return true;
    }
    // Main address failed, check the alternate address
    if (alt_addr && i2c_read_byte(ASDA, ASCL, alt_addr, reg) == expected) {
        driver->addr = alt_addr;
        return true;
    }
    return false;
}

bool init_driver(const FusionDevice *device, FusionDriver *driver, const char *name) {
    if (!driver || !driver->exists(driver)) {
        return false;
    }
    if (!driver->init(driver)) {
        printsys(imu, "WARNING: detected %s for '%s' but failed to initialize", name, device->name);
        return false;
    }
    printsys(imu, "initialized %s for '%s'", name, device->name);
    return true;
}

void deinit_driver(const FusionDevice *device, FusionDriver *driver, const char *name) {
    if (driver && driver->destroy) {
        driver->destroy(driver);
        printsys(imu, "deinitialized %s for '%s'", name, device->name);
    }
}
