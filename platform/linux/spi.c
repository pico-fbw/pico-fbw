/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "platform/defs.h"

#include "platform/spi.h"

typedef struct SPIInstance {
    const char *device;
    int fd;
    u32 freq;
} SPIInstance;

static SPIInstance instances[MAX_SPI_DEVICES];

/**
 * @return a pointer to the SPI instance that uses the given pins, or NULL if no such instance exists
 */
static SPIInstance *spi_instance_from_pins(i16 clk, i16 mosi, i16 miso, i16 cs) {
    for (u32 i = 0; i < MAX_SPI_DEVICES; i++) {
        SPIMapping mapping = SPI_MAP[i];
        if (mapping.clk == clk && mapping.mosi == mosi && mapping.miso == miso && mapping.cs == cs) {
            instances[i].device = mapping.device;
            return &instances[i];
        }
    }
    return NULL;
}

bool spi_setup(i16 clk, i16 mosi, i16 miso, u32 freq) {
    // Set up every cs line that shares these clk/mosi/miso pins, since spi_transfer()
    // may be called with any of them and each maps to a distinct spidev fd
    bool anySetUp = false;
    for (u32 i = 0; i < MAX_SPI_DEVICES; i++) {
        SPIMapping mapping = SPI_MAP[i];
        if (mapping.clk != clk || mapping.mosi != mosi || mapping.miso != miso) {
            continue;
        }
        instances[i].device = mapping.device;

        // Found a match to these pins, claim the fd
        instances[i].fd = open(instances[i].device, O_RDWR);
        if (instances[i].fd < 0) {
            continue;
        }
        // Set mode 0 and 8-bit mode
        u8 mode = SPI_MODE_0;
        u8 bits = 8;
        if (ioctl(instances[i].fd, SPI_IOC_WR_MODE, &mode) < 0 ||
            ioctl(instances[i].fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ||
            ioctl(instances[i].fd, SPI_IOC_WR_MAX_SPEED_HZ, &freq) < 0) {
            close(instances[i].fd);
            instances[i].fd = -1;
            continue;
        }
        instances[i].freq = freq;
        anySetUp = true;
    }
    return anySetUp;
}

bool spi_transfer(i16 clk, i16 mosi, i16 miso, i16 cs, const byte tx[], byte rx[], size_t len) {
    SPIInstance *inst = spi_instance_from_pins(clk, mosi, miso, cs);
    if (inst == NULL || inst->fd < 0) {
        return false;
    }

    struct spi_ioc_transfer xfer = {};
    xfer.tx_buf = (unsigned long long)tx;
    xfer.rx_buf = (unsigned long long)rx;
    xfer.len = len;
    xfer.speed_hz = inst->freq;
    xfer.bits_per_word = 8;
    // cs is asserted automatically by the kernel driver based on the spidev device's cs line,
    // which is handled by the instance lookup
    return ioctl(inst->fd, SPI_IOC_MESSAGE(1), &xfer) >= 0;
}
