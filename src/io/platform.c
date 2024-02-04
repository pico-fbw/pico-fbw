/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * 
 * This file utilizes code under the BSD-3-Clause License. See "LICENSE" for details.
*/

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

// TODO: refactor platform into larger HAL; stray away from pico functions to make easier to port, maybe add just a bit of compile-time config back?

#include <stdio.h>
#include <string.h>
#include "pico/bootrom.h"
#include "pico/time.h"
#include "pico/types.h"

#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"

#include "io/aahrs.h"
#include "io/display.h"
#include "io/flash.h"
#include "io/gps.h"
#include "io/pwm.h"

#include "modes/aircraft.h"

#include "sys/api/api.h"
#include "sys/info.h"
#include "sys/log.h"
#include "sys/switch.h"

#include "io/platform.h"

/** @section Boot functions */

bool isBooted = false;

void platform_boot_begin() {
    info_declare();
    isBooted = false;
    gpio_pull_down(22);
    if (platform_is_fbw()) display_init();
}

void platform_boot_setProgress(float progress, const char *message) {
    printf("[boot] (%.1f%%) %s\n", progress, message);
    if (platform_is_fbw()) {
        display_string(message, (int)progress);
    }
}

void platform_enable_watchdog() {
    if ((int)flash.system[SYSTEM_WATCHDOG_TIMEOUT] > 0) {
        watchdog_enable((uint32_t)flash.system[SYSTEM_WATCHDOG_TIMEOUT], true);
        watchdog_hw->scratch[0] = WATCHDOG_TIMEOUT_MAGIC;
    }
}

bool platform_is_booted() { return isBooted; }

void platform_boot_complete() {
    if (platform_is_fbw() && log_countErrs() == 0) display_anim();
    isBooted = true;
}

/** @section Power functions */

BootType platform_boot_type() {
    if (watchdog_caused_reboot()) {
        // If the reboot was intentional (forced by firmware or API), WATCHDOG_FORCE_MAGIC would have been set
        if (watchdog_hw->scratch[0] == WATCHDOG_FORCE_MAGIC) {
            return BOOT_REBOOT;
        } else if (watchdog_hw->scratch[0] == WATCHDOG_TIMEOUT_MAGIC) {
            return BOOT_WATCHDOG; // Not good...watchdog had to reboot while program was running (after boot)
        } else {
            return BOOT_BOOTSEL; // Watchdog caused reboot before pico-fbw finished booting, likely BOOTSEL (it uses watchdog)
        }
    } else {
        return BOOT_COLD;
    }
}

void __attribute__((noreturn)) platform_reboot(RebootType type) {
    switch (type) {
        case REBOOT_FAST:
            watchdog_hw->scratch[0] = WATCHDOG_FORCE_MAGIC;
            watchdog_hw->ctrl = WATCHDOG_CTRL_TRIGGER_BITS;
            break;
        case REBOOT_BOOTLOADER:
            reset_usb_boot(0, 0);
    }
    while (true) tight_loop_contents(); // Stall for impending reboot
}

void __attribute__((noreturn)) platform_shutdown() {
    reset_usb_boot(0, 1); // Reboot into bootloader but don't mount mass storage
}

/** @section Runtime functions */

void platform_loop(bool updateAircraft) {
    // Update the mode switch's position, update sensors, run the current mode's code, respond to any new API calls, and update the watchdog
    float switchPos = pwm_read((uint)flash.pins[PINS_INPUT_SWITCH], PWM_MODE_DEG);
    switch ((SwitchType)flash.general[GENERAL_SWITCH_TYPE]) {
        case SWITCH_TYPE_2_POS:
            if (switchPos < 90) {
                switch_update(SWITCH_POSITION_LOW);
            } else {
                switch_update(SWITCH_POSITION_HIGH);
            }
            break;
        case SWITCH_TYPE_3_POS:
            if (switchPos < 45) {
                switch_update(SWITCH_POSITION_LOW);
            } else if (switchPos > 135) {
                switch_update(SWITCH_POSITION_HIGH);
            } else {
                switch_update(SWITCH_POSITION_MID);
            }
            break;
    }
    aahrs.update();
    if (gps.isSupported()) gps.update();
    if (updateAircraft) aircraft.update();
    if ((bool)flash.general[GENERAL_API_ENABLED]) api_poll();
    watchdog_update();
}

void platform_sleep_ms(uint32_t ms, bool updateAircraft) {
    absolute_time_t wakeup_time = make_timeout_time_ms(ms);
    while (!time_reached(wakeup_time)) {
        platform_loop(updateAircraft);
    }
}

bool __no_inline_not_in_flash_func(platform_buttonPressed)() {
    const uint CS_PIN_INDEX = 1;
    uint32_t intr = save_and_disable_interrupts();
    // Set chip select to Hi-Z
    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                    GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);
    for (volatile uint i = 0; i < 1000; i++); // (Can't sleep)
    // Note the button pulls the pin on QSPI low when pressed.
    bool btnState = !(sio_hw->gpio_hi_in & (1u << CS_PIN_INDEX));
    // Restore the state of chip select
    hw_write_masked(&ioqspi_hw->io[CS_PIN_INDEX].ctrl,
                    GPIO_OVERRIDE_NORMAL << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                    IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);
    restore_interrupts(intr);
    return btnState;
}

/** @section Platform type functions */

bool platform_is_pico() {
    #if defined(RASPBERRYPI_PICO) || defined(RASPBERRYPI_PICO_W)
        return true;
    #else
        return false;
    #endif
}

bool platform_is_fbw() { return gpio_get(22); }

Platform platform_type() {
    if (platform_is_fbw()) {
        return PLATFORM_FBW;
    } else if (platform_is_pico()) {
        #if defined(RASPBERRYPI_PICO)
            return PLATFORM_PICO;
        #elif defined(RASPBERRYPI_PICO_W)
            return PLATFORM_PICO_W;
        #endif
    } else {
        return PLATFORM_UNKNOWN;
    }
}
