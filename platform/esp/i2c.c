/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <string.h>
#include "driver/i2c_master.h" // https://docs.espressif.com/projects/esp-idf/en/v5.2/esp32/api-reference/peripherals/i2c.html

#include "platform/helpers.h"

#include "platform/i2c.h"

// Interrupt WDT had to be increased in sdkconfig to prevent it from triggering during long timeouts
#define I2C_TIMEOUT_MS 50     // Timeout for all I2C operations
#define MAX_DEVICES_PER_BUS 5 // Maximum number of devices that can exist on a single bus

typedef struct I2CDevice {
    byte addr;
    i2c_master_dev_handle_t handle;
} I2CDevice;

typedef struct I2CBus {
    i16 sda, scl;
    u32 freq;
    i2c_master_bus_handle_t handle;
    I2CDevice devices[MAX_DEVICES_PER_BUS];
    size_t numDevices;
} I2CBus;

static I2CBus buses[I2C_NUM_MAX];

/**
 * Adds an `I2CDevice` to the given `I2CBus`.
 * @param bus the `I2CBus` to add the device to
 * @param addr the I2C address of the device
 * @return the `I2CDevice` that was added to the bus, or NULL if the device could not be added
 */
static I2CDevice *add_device(I2CBus *bus, byte addr) {
    if (++bus->numDevices > MAX_DEVICES_PER_BUS) {
        bus->numDevices--;
        return NULL;
    }
    // Add the new device to the bus
    const i2c_device_config_t deviceConfig = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = (u16)addr,
        .scl_speed_hz = bus->freq,
    };
    i2c_master_dev_handle_t deviceHandle;
    if (i2c_master_bus_add_device(bus->handle, &deviceConfig, &deviceHandle) != ESP_OK) {
        return NULL;
    }
    bus->devices[bus->numDevices - 1] = (I2CDevice){addr, deviceHandle};
    return &bus->devices[bus->numDevices - 1];
}

/**
 * Returns the `I2CDevice` that matches the given details.
 * @param sda the SDA pin that the device is connected to
 * @param scl the SCL pin that the device is connected to
 * @param addr the I2C address of the device
 * @return the `I2CDevice` that matches the given details,
 * or NULL if no such bus exists matching the given SDA and SCL pins
 * @note If no such device exists, it will be automatically added to the bus.
 */
static I2CDevice *i2c_device_from_details(i16 sda, i16 scl, byte addr) {
    // Find the bus that matches the given SDA and SCL pins
    I2CBus *bus = NULL;
    for (size_t i = 0; i < count_of(buses); i++) {
        if (buses[i].sda == sda && buses[i].scl == scl) {
            bus = &buses[i];
            break;
        }
    }
    if (!bus) {
        return NULL; // No bus found
    }
    // Now, find the device that matches the given address
    I2CDevice *device = NULL;
    for (size_t i = 0; i < bus->numDevices; i++) {
        if (bus->devices[i].addr == addr) {
            device = &bus->devices[i];
            break;
        }
    }
    if (!device) {
        // Device has not yet been added to the bus, try to do that now
        device = add_device(bus, addr);
        if (!device) {
            return NULL;
        }
    }
    return device;
}

bool i2c_setup(i16 sda, i16 scl, u32 freq) {
    const i2c_master_bus_config_t config = {
        .i2c_port = -1, // auto-select
        .sda_io_num = sda,
        .scl_io_num = scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .flags.enable_internal_pullup = true,
    };
    i2c_master_bus_handle_t handle;
    if (i2c_new_master_bus(&config, &handle) != ESP_OK) {
        return false;
    }
    // Add the initialized bus to the array
    for (size_t i = 0; i < count_of(buses); i++) {
        if (!buses[i].handle) {
            buses[i].sda = sda;
            buses[i].scl = scl;
            buses[i].freq = freq;
            buses[i].handle = handle;
            return true;
        }
    }
    return false;
}

bool i2c_read(i16 sda, i16 scl, byte addr, byte reg, byte dest[], size_t len) {
    I2CDevice *device = i2c_device_from_details(sda, scl, addr);
    if (!device) {
        return false;
    }
    return i2c_master_transmit_receive(device->handle, &reg, sizeof(reg), dest, len, I2C_TIMEOUT_MS) == ESP_OK;
}

bool i2c_write(i16 sda, i16 scl, byte addr, byte reg, const byte src[], size_t len) {
    I2CDevice *device = i2c_device_from_details(sda, scl, addr);
    if (!device) {
        return false;
    }
    // Prefix the register to the source data
    byte cmd[len + 1];
    cmd[0] = reg;
    memcpy(cmd + 1, src, len);
    return i2c_master_transmit(device->handle, cmd, len + 1, I2C_TIMEOUT_MS) == ESP_OK;
}
