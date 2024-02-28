/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#include "pico/config.h"
#include "pico/platform.h"
#include "pico/stdio.h"
#ifdef RASPBERRYPI_PICO_W
#include "pico/cyw43_arch.h"
#endif

#include "platform/sys.h"

#define WATCHDOG_TIMEOUT_MS 2000
#define WATCHDOG_TIMEOUT_MAGIC 0xAC0B3DED
#define WATCHDOG_FORCE_MAGIC 0xC0DE3298

void sys_boot_begin() {
    stdio_init_all(); // The stdio types that are initializes here depend on what gets defined in platform/pico/CMakeLists.txt
#ifdef RASPBERRYPI_PICO_W
    cyw43_arch_init();
#endif
}

void sys_boot_end() {
    // Enable watchdog and set a magic value to indicate we've taken control of the watchdog
    watchdog_enable(WATCHDOG_TIMEOUT_MS, true);
    watchdog_hw->scratch[0] = WATCHDOG_TIMEOUT_MAGIC;
}

void sys_periodic() { watchdog_update(); }

void __attribute__((noreturn)) sys_shutdown() {
    reset_usb_boot(0, 1); // Reboot into bootloader but don't mount mass storage
}

void __attribute__((noreturn)) sys_reboot(bool bootloader) {
    if (bootloader) {
        reset_usb_boot(0, 0); // Reboot into bootloader and mount mass storage
    }
    // Set a magic value in the scratch register to indicate that the reboot was intentional (since power-on reset doesn't clear
    // the scratch register)
    watchdog_hw->scratch[0] = WATCHDOG_FORCE_MAGIC;
    // Trigger the watchdog to force a reboot and stall for it to take effect
    watchdog_hw->ctrl = WATCHDOG_CTRL_TRIGGER_BITS;
    while (true)
        tight_loop_contents();
}

BootType sys_boot_type() {
    if (watchdog_caused_reboot()) {
        // If the reboot was intentional (forced by firmware), WATCHDOG_FORCE_MAGIC would have been set into scratch[0] before
        // triggering the watchdog
        if (watchdog_hw->scratch[0] == WATCHDOG_FORCE_MAGIC) {
            return BOOT_REBOOT;
        } else if (watchdog_hw->scratch[0] == WATCHDOG_TIMEOUT_MAGIC) {
            return BOOT_WATCHDOG; // This flag is set after the boot finishes, which means watchdog had to reboot while program
                                  // was running...not good
        } else {
            return BOOT_RESET; // No flag was set, so watchdog caused reboot before it was enabled, likely BOOTSEL (it uses
                               // watchdog and doesn't set that flag)
        }
    } else
        return BOOT_COLD; // No watchdog reboot, so a cold boot
}
