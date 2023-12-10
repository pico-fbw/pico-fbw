/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * 
 * This file utilizes code under the BSD-3-Clause License. See "LICENSE" for details.
*/

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "pico/bootrom.h"
#include "pico/time.h"

#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/watchdog.h"
#include "hardware/structs/ioqspi.h"
#include "hardware/structs/sio.h"

#include "aahrs.h"
#include "display.h"
#include "flash.h"
#include "gps.h"
#include "pwm.h"

#include "../modes/modes.h"

#include "../sys/api/api.h"
#include "../sys/info.h"
#include "../sys/log.h"
#include "../sys/switch.h"

#include "platform.h"

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
        // Create four lines of text characters to later send to the display
        char line1[DISPLAY_MAX_LINE_LEN] = { [0 ... DISPLAY_MAX_LINE_LEN - 1] = ' '};
        char line2[DISPLAY_MAX_LINE_LEN] = { [0 ... DISPLAY_MAX_LINE_LEN - 1] = ' '};
        char line3[DISPLAY_MAX_LINE_LEN] = { [0 ... DISPLAY_MAX_LINE_LEN - 1] = ' '};
        char line4[DISPLAY_MAX_LINE_LEN] = { [0 ... DISPLAY_MAX_LINE_LEN - 1] = ' '};
        display_pBarStr(line4, (uint)progress);

        uint numWords = 1;
        for (uint i = 0; message[i] != '\0'; i++) {
            if (message[i] == ' ' && message[i + 1] != ' ') numWords++;    
        }
        if (numWords <= 3) {
            // Each word can probably fit on one line
            // First split into each line
            char messageCopy[strlen(message) + 1];
            strcpy(messageCopy, message);
            uint wordCount = 0;
            char *token = strtok(messageCopy, " ");
            while (token != NULL && wordCount < 3) {
                if (wordCount == 0) {
                    if (strlen(token) <= DISPLAY_MAX_LINE_LEN) {
                        strncpy(line1, token, DISPLAY_MAX_LINE_LEN);
                    } else {
                        // Word was longer than the line
                        goto split;
                    }
                } else if (wordCount == 1) {
                    if (strlen(token) <= DISPLAY_MAX_LINE_LEN) {
                        strncpy(line2, token, DISPLAY_MAX_LINE_LEN);
                    } else {
                        goto split;
                    }
                } else if (wordCount == 2) {
                    if (strlen(token) <= DISPLAY_MAX_LINE_LEN) {
                        strncpy(line3, token, DISPLAY_MAX_LINE_LEN);
                    } else {
                        goto split;
                    }
                }
                token = strtok(NULL, " ");
                wordCount++;
            }
        } else {
            // Split words between lines irregularly; we can't fit each neatly on its own line
            split: {
                uint numLines = (strlen(message) + (DISPLAY_MAX_LINE_LEN - 1)) / DISPLAY_MAX_LINE_LEN;
                switch (numLines) {
                    case 1:
                        strncpy(line1, message, DISPLAY_MAX_LINE_LEN);
                        break;
                    case 2:
                        strncpy(line1, message, DISPLAY_MAX_LINE_LEN);
                        strncpy(line2, message + DISPLAY_MAX_LINE_LEN, DISPLAY_MAX_LINE_LEN);
                        break;
                    case 3:
                        strncpy(line1, message, DISPLAY_MAX_LINE_LEN);
                        strncpy(line2, message + DISPLAY_MAX_LINE_LEN, DISPLAY_MAX_LINE_LEN);
                        strncpy(line3, message + (DISPLAY_MAX_LINE_LEN * 2), DISPLAY_MAX_LINE_LEN);
                        break;
                    default:
                        return; // Won't fit at all
                }
            }
        }
        display_text(line1, line2, line3, line4, true);
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
