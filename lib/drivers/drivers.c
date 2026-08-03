/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <string.h>
#include "platform/helpers.h"
#include "platform/i2c.h"
#include "platform/spi.h"

#include "sys/print.h"

#include "drivers.h"

// Master device list; devices are defined in their respective driver source files
FusionDevice *fusionDevices[] = {
    &bmi323,
};
const u32 numFusionDevices = count_of(fusionDevices);

static bool check_devid_i2c(BusConfig *bus, byte addr, byte alt_addr, byte reg, byte expected,
                            check_devid_fn check_fn) {
    bus->type = BUS_I2C;
    bus->i2c.addr = addr;
    if (check_fn(bus, reg) == expected) {
        return true;
    }
    // Main address failed, check the alternate address
    bus->i2c.addr = alt_addr;
    return (alt_addr && check_fn(bus, reg) == expected);
}

static bool check_devid_spi(BusConfig *bus, byte reg, byte expected, check_devid_fn check_fn) {
    const i16 csPins[] = {
        (i16)config.pins.spiCs0,
        (i16)config.pins.spiCs1,
        (i16)config.pins.spiCs2,
    };
    // Determine how many CS pins are valid
    u8 numCsPins = 0;
    for (size_t i = 0; i < count_of(csPins); i++) {
        if (csPins[i] >= 0) {
            numCsPins++;
        } else {
            break;
        }
    }

    bus->type = BUS_SPI;
    for (u32 i = 0; i < numCsPins; i++) {
        bus->spi.cs = csPins[i];
        if (check_fn(bus, reg) == expected) {
            return true;
        }
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

bool check_devid(BusConfig *bus, byte addr, byte alt_addr, byte reg, byte expected, check_devid_fn check_fn) {
    switch ((BusType)config.sensors.busType) {
        case BUS_ALL:
        case BUS_I2C:
            if (check_devid_i2c(bus, addr, alt_addr, reg, expected, check_fn)) {
                return true;
            }
            if ((BusType)config.sensors.busType == BUS_I2C) {
                return false;
            }
            /* fall through */
        case BUS_SPI:
            return check_devid_spi(bus, reg, expected, check_fn);
    }
    return false;
}

bool driver_read(BusConfig *bus, byte reg, byte dest[], size_t len) {
    switch (bus->type) {
        case BUS_ALL:
            break;
        case BUS_I2C:
            return i2c_read((i16)config.pins.i2cSda, (i16)config.pins.i2cScl, bus->i2c.addr, reg, dest, len);
        case BUS_SPI:
            byte tx[1 + len];
            byte rx[1 + len];
            tx[0] = reg | 0x80; // Set read bit
            memset(&tx[1], 0, len);
            if (!spi_transfer((i16)config.pins.spiClk, (i16)config.pins.spiMosi, (i16)config.pins.spiMiso, bus->spi.cs,
                              tx, rx, 1 + len)) {
                return false;
            }
            memcpy(dest, &rx[1], len);
            return true;
    }
    return false;
}

bool driver_write(BusConfig *bus, byte reg, const byte src[], size_t len) {
    switch (bus->type) {
        case BUS_ALL:
            break;
        case BUS_I2C:
            return i2c_write((i16)config.pins.i2cSda, (i16)config.pins.i2cScl, bus->i2c.addr, reg, src, len);
        case BUS_SPI:
            byte tx[1 + len];
            byte rx[1 + len];   // This will be discarded, but spi_transfer needs a buffer
            tx[0] = reg & 0x7F; // Set write bit
            memcpy(&tx[1], src, len);
            return spi_transfer((i16)config.pins.spiClk, (i16)config.pins.spiMosi, (i16)config.pins.spiMiso,
                                bus->spi.cs, tx, rx, 1 + len);
    }
    return false;
}
