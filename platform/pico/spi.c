/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "hardware/spi.h"
#include "hardware/gpio.h"

#include "platform/spi.h"

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
}

bool spi_setup(i16 clk, i16 mosi, i16 miso, u32 freq) {
    gpio_set_function(clk, GPIO_FUNC_SPI);
    gpio_set_function(mosi, GPIO_FUNC_SPI);
    gpio_set_function(miso, GPIO_FUNC_SPI);
    spi_inst_t *spi = spi_inst_from_pins(clk, mosi, miso);
    if (!spi) {
        return false;
    }
    spi_init(spi, freq);
    return true;
}

bool spi_transfer(i16 clk, i16 mosi, i16 miso, i16 cs, const byte tx[], byte rx[], size_t len) {
    spi_inst_t *spi = spi_inst_from_pins(clk, mosi, miso);
    if (!spi) {
        return false;
    }
    return spi_write_read_blocking(spi, tx, rx, len) == len;
}
