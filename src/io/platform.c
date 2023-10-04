/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include "pico/bootrom.h"
#include "pico/config.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"

#include "../sys/info.h"
#include "../sys/config.h"

#include "platform.h"

void platform_boot_begin() {
    info_declare();
    gpio_pull_down(22);
    if (platform_is_fbw()) {
        // TODO: logo now
        
    }
}

void platform_boot_setProgress(float progress, const char *message) {
    printf("[boot] (%.1f%%) %s\n", progress, message);
    if (platform_is_fbw()) {
        // TODO: progress indicator now

    }
}

void platform_boot_complete() {
    printf("[boot] bootup complete!\n");
    watchdog_hw->scratch[0] = WATCHDOG_TIMEOUT_MAGIC;
    if (platform_is_fbw()) {
        // TODO: boot complete animation now

    }
}

void platform_reboot(RebootType type) {
    switch (type) {
        case REBOOT_FAST:
            watchdog_hw->scratch[0] = WATCHDOG_FORCE_MAGIC;
            watchdog_hw->ctrl = WATCHDOG_CTRL_TRIGGER_BITS;
            break;
        case REBOOT_BOOTLOADER:
            reset_usb_boot(0, 0);
            break;
    }
    while (true) tight_loop_contents(); // Stall for impending reboot
}

void platform_shutdown() {
    reset_usb_boot(0, 1); // Reboot into bootloader but don't mount mass storage
    while (true) tight_loop_contents();
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
