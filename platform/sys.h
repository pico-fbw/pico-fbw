#pragma once

#include <stdbool.h>

typedef enum BootType {
    BOOT_COLD,     // Cold boot (power-on reset)
    BOOT_REBOOT,   // Reboot (intentional), caused by running sys_reset()
    BOOT_RESET,    // Reset (e.g. by launching a program after loading it from the bootloader)
    BOOT_WATCHDOG, // Watchdog reboot, caused by not updating the watchdog in time (a crash likely occured)
} BootType;

/**
 * Will be called at the beginning of the boot process; when the system is powered on.
 */
void sys_boot_begin();

/**
 * Will be called once the boot process is complete, but before the main program loop is entered.
 */
void sys_boot_end();

/**
 * Will be called periodically from the main program loop.
 */
void sys_periodic();

/**
 * Shuts down the system.
 */
void __attribute__((noreturn)) sys_shutdown();

/**
 * Reboots the system.
 * @param bootloader if true, the system will reboot into the bootloader
 */
void __attribute__((noreturn)) sys_reboot(bool bootloader);

/**
 * @return the type of boot that just occurred
 * @note Must be called between `sys_boot_begin()` and `sys_boot_complete()`.
 */
BootType sys_boot_type();
