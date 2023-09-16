/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include "pico/config.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "error.h"

#include "../sys/info.h"

#include "platform.h"

static bool isAthena = false;

const unsigned char bt_anim[] = {
    0x20,
    0b00000000, 0b00111111,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 5,
    0b00000000, 0b00111111,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 5, 0,
    0b00000000, 0b00111111,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 0, 0,
    0b00000000, 0b00111111,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 0, 0,
    0b00000000, 0b00111111,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 0, 0,
    0b00000000, 0b00111111,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 5, 0,
    0b00000000, 0b00111111,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 5,
    0b00000000, 0b00111111,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 5
};

const unsigned char st_anim[] = {
    0x20,
    0b00000000, 0b00111111,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0,
    0b00000000, 0b00111111,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 0, 0, 0, 0,
    0b00000000, 0b00111111,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 5, 0, 0, 0,
    0b00000000, 0b00111111,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 5, 5, 0, 0,
    0b00000000, 0b00111111,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 5, 5, 5, 0,
    0b00000000, 0b00111111,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 5, 5, 5, 5, 5
};

static inline bool marbe_w(unsigned char addr, unsigned char val) {
    unsigned char c[2] = {addr, val};
    return i2c_write_timeout_us(MARBE_I, MARBE_R, c, 2, true, MARBE_TUS);
}

static inline void marbe_anim(bool anim) {
    anim ? marbe_w(0x00, 0b00100000) : marbe_w(0x00, 0b00000000);
}

static void marbe_i() {
    i2c_init(MARBE_I, MARBE_FKZ * 1000);
    gpio_set_function(MARBE_D, GPIO_FUNC_I2C);
    gpio_set_function(MARBE_C, GPIO_FUNC_I2C);
    gpio_pull_up(MARBE_D);
    gpio_pull_up(MARBE_C);
    marbe_w(0x00, 0b00000000);
    marbe_w(0x01, 0b00000000);
    marbe_w(0x02, 0b00111111);
    marbe_w(0x03, 0b00111000);
    marbe_w(0xB6, 0b10000000);
}

void marbe_s(uint8_t l1, uint8_t l2, uint8_t l3, uint8_t l4, uint8_t l5, uint8_t l6) {
    marbe_w(0x1F, l1);
    marbe_w(0x1E, l2);
    marbe_w(0x1D, l3);
    marbe_w(0x1C, l4);
    marbe_w(0x1B, l5);
    marbe_w(0x1A, l6);
    marbe_w(0xB0, 0);
}

void platform_boot_begin() {
    info_declare();
    gpio_pull_down(22);
    if (platform_is_fbw()) {
        marbe_i();
        i2c_write_blocking(MARBE_I, MARBE_R, bt_anim, sizeof(bt_anim), true);
        marbe_anim(true);
    }
    led_init();
}

void platform_boot_complete() {
    if (platform_is_fbw()) {
        // TODO: this being interrupted by error.c?
        i2c_write_blocking(MARBE_I, MARBE_R, st_anim, sizeof(st_anim), true);
        sleep_ms(420);
        marbe_anim(false);
        marbe_s(1, 1, 1, 1, 1, 1);
    }
}

/* ---------------------------------------- */

int platform_type() {
    if (platform_is_athena()) {
        return 3;
    } else if (platform_is_fbw()) {
        return 2;
    } else if (platform_is_pico()) {
        #if defined(RASPBERRYPI_PICO)
            return 0;
        #elif defined(RASPBERRYPI_PICO_W)
            return 1;
        #else
            return -1;
        #endif
    } else {
        return -1;
    }
}

bool platform_is_pico() {
    #if defined(RASPBERRYPI_PICO) || defined(RASPBERRYPI_PICO_W)
        return true;
    #else
        return false;
    #endif
}

bool platform_is_fbw() {
    return platform_is_athena() ? true : gpio_get(22);
}

bool platform_is_athena() { return (isAthena && gpio_get(22)); }
