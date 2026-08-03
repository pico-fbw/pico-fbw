/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <assert.h>
#include "platform/helpers.h"

#include "hardware/gpio.h"
#include "hardware/spi.h"

#include "platform/spi.h"

#define MAX_CS_PINS 10
static i16 inittedCsPins[] = {[0 ... MAX_CS_PINS - 1] = -1};

/**
 * @return the SPI instance that the given pins lie on, or NULL if the pins do not form a valid SPI instance
 */
static inline spi_inst_t *spi_inst_from_pins(i16 clk, i16 mosi, i16 miso) {
    switch (clk) {
        case 2:
        case 6:
        case 18:
            switch (mosi) {
                case 3:
                case 7:
                case 19:
                    switch (miso) {
                        case 0:
                        case 4:
                        case 16:
                        case 20:
                            return spi0;
                    }
                    break;
                default:
                    return NULL;
            }
            break;
        case 10:
        case 14:
        case 26:
            switch (mosi) {
                case 11:
                case 15:
                case 27:
                    switch (miso) {
                        case 8:
                        case 12:
                        case 28:
                            return spi1;
                    }
                    break;
                default:
                    return NULL;
            }
            break;
        default:
            return NULL;
    }
    return NULL;
}

// Ensures that the given CS pin has been initialized for output.
static void ensure_cs(i16 cs) {
    for (u32 i = 0; i < count_of(inittedCsPins); i++) {
        if (inittedCsPins[i] == cs) {
            return;
        }
    }
    // This CS pin has never been initialized; do that now and add it into the initted array
    gpio_init(cs);
    gpio_set_dir(cs, GPIO_OUT);
    gpio_put(cs, 1);
    for (u32 i = 0; i < count_of(inittedCsPins); i++) {
        if (inittedCsPins[i] < 0) {
            inittedCsPins[i] = cs;
            return;
        }
    }
    assert(false); // This will only run if we run out of CS pins for some reason
}

bool spi_setup(i16 clk, i16 mosi, i16 miso, u32 freq) {
    spi_inst_t *spi = spi_inst_from_pins(clk, mosi, miso);
    if (!spi) {
        return false;
    }
    gpio_set_function(clk, GPIO_FUNC_SPI);
    gpio_set_function(mosi, GPIO_FUNC_SPI);
    gpio_set_function(miso, GPIO_FUNC_SPI);
    spi_init(spi, freq);
    return true;
}

bool spi_transfer(i16 clk, i16 mosi, i16 miso, i16 cs, const byte tx[], byte rx[], size_t len) {
    spi_inst_t *spi = spi_inst_from_pins(clk, mosi, miso);
    if (!spi) {
        return false;
    }
    ensure_cs(cs);
    gpio_put(cs, 0); // Assert CS line; active low
    bool ok = spi_write_read_blocking(spi, tx, rx, len) == (i32)len;
    gpio_put(cs, 1);
    return ok;
}
