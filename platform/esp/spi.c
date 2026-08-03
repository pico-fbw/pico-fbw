/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <string.h>
#include "driver/spi_master.h" // https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/spi_master.html

#include "platform/helpers.h"

#include "platform/spi.h"

// The structure of this file is similar to i2c.c, see that for more comments on its workings

#define MAX_DEVICES_PER_BUS 5

typedef struct SPIDevice {
    i16 cs;
    spi_device_handle_t handle;
} SPIDevice;

typedef struct SPIBus {
    i16 clk, mosi, miso;
    u32 freq;
    spi_host_device_t host;
    SPIDevice devices[MAX_DEVICES_PER_BUS];
    size_t numDevices;
} SPIBus;

static SPIBus buses[SOC_SPI_PERIPH_NUM - 1] = {}; // SPI1 is unsupported

/**
 * Adds a `SPIDevice` to the given `SPIBus`.
 * @param bus the `SPIBus` to add the device to
 * @param cs the chip select pin of the device
 * @return the `SPIDevice` that was added to the bus, or NULL if the device could not be added
 */
static SPIDevice *add_device(SPIBus *bus, i16 cs) {
    if (++bus->numDevices > MAX_DEVICES_PER_BUS) {
        bus->numDevices--;
        return NULL;
    }
    const spi_device_interface_config_t deviceConfig = {
        .mode = 0,
        .clock_speed_hz = (int)bus->freq,
        .spics_io_num = cs,
        .queue_size = 7,
    };
    spi_device_handle_t deviceHandle;
    if (spi_bus_add_device(bus->host, &deviceConfig, &deviceHandle) != ESP_OK) {
        return NULL;
    }
    bus->devices[bus->numDevices - 1] = (SPIDevice){cs, deviceHandle};
    return &bus->devices[bus->numDevices - 1];
}

/**
 * Returns the `SPIDevice` that matches the given details.
 * @param clk the CLK pin of the device's SPI bus
 * @param mosi the MOSI pin of the device's SPI bus
 * @param miso the MISO pin of the device's SPI bus
 * @param cs the CS pin of the device
 * @return the `SPIDevice` that matches the given details,
 * or NULL if no such bus exists matching the given pins
 * @note If no such device exists, it will be automatically added to the bus.
 */
static SPIDevice *device_from_details(i16 clk, i16 mosi, i16 miso, i16 cs) {
    SPIBus *bus = NULL;
    for (size_t i = 0; i < count_of(buses); i++) {
        if (buses[i].clk == clk && buses[i].mosi == mosi && buses[i].miso == miso) {
            bus = &buses[i];
            break;
        }
    }
    if (!bus) {
        return NULL;
    }

    SPIDevice *device = NULL;
    for (size_t i = 0; i < bus->numDevices; i++) {
        if (bus->devices[i].cs == cs) {
            device = &bus->devices[i];
            break;
        }
    }
    if (!device) {
        device = add_device(bus, cs);
        if (!device) {
            return NULL;
        }
    }
    return device;
}

bool spi_setup(i16 clk, i16 mosi, i16 miso, u32 freq) {
    // Automatically select the lowest available SPI host (starting at SPI2 since SPI1 is unavailable)
    spi_host_device_t availHost = SPI2_HOST;
    for (size_t i = 0; i < count_of(buses); i++) {
        if (buses[i].host >= availHost) {
            availHost = buses[i].host + 1;
        }
    }

    const spi_bus_config_t config = {
        .mosi_io_num = mosi,
        .miso_io_num = miso,
        .sclk_io_num = clk,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0, // auto-set
    };
    if (spi_bus_initialize(availHost, &config, SPI_DMA_CH_AUTO) != ESP_OK) {
        return false;
    }

    for (size_t i = 0; i < count_of(buses); i++) {
        SPIBus bus = buses[i];
        if (bus.host == 0) {
            bus.clk = clk;
            bus.mosi = mosi;
            bus.miso = miso;
            bus.freq = freq;
            bus.host = availHost;
        }
        return true;
    }
    return false;
}

bool spi_transfer(i16 clk, i16 mosi, i16 miso, i16 cs, const byte tx[], byte rx[], size_t len) {
    SPIDevice *device = device_from_details(clk, mosi, miso, cs);
    if (!device) {
        return false;
    }
    spi_transaction_t trans = {
        .length = len * 8, // length is specified in bits
        .tx_buffer = tx,
        .rx_buffer = rx,
    };
    return spi_device_polling_transmit(device->handle, &trans) == ESP_OK;
}
