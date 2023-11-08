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
#include "hardware/watchdog.h"

#include "display.h"

#include "../sys/info.h"

#include "platform.h"

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

bool platform_is_booted() { return isBooted; }

void platform_boot_complete() {
    watchdog_hw->scratch[0] = WATCHDOG_TIMEOUT_MAGIC;
    if (platform_is_fbw()) display_anim();
    isBooted = true;
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

BootType platform_boot_type() {
    if (watchdog_enable_caused_reboot()) {
        // If the reboot was intentional (forced), WATCHDOG_FORCE_MAGIC would have been set
        if (watchdog_hw->scratch[0] == WATCHDOG_FORCE_MAGIC) {
            return BOOT_REBOOT;
        } else if (watchdog_hw->scratch[0] == WATCHDOG_TIMEOUT_MAGIC) {
            return BOOT_WATCHDOG;
        } else {
            return BOOT_NORMAL; // Watchdog caused reboot before the system finished booting, likely BOOTSEL
        }
    } else {
        return BOOT_NORMAL;
    }
}

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
