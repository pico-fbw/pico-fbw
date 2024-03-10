/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/sys.h"

void sys_boot_begin() {
    // This code will be executed at the beginning of the boot process when the system is powered on.
    // Use it to initialize any platform-specific critical hardware or software that must be set up before anything else can
    // happen. If your platform doesn't need to do anything special at boot, you can leave this function empty.
}

void sys_boot_end() {
    // This code will be executed once the boot process is complete, but before the main program loop is entered.
    // Use it to perform any final setup or initialization that must occur before the main program loop starts.
    // If your platform has a watchdog timer, it is advisable to start it here.
    // If your platform doesn't need to do anything special at end of boot, you can leave this function empty.
}

void sys_periodic() {
    // This code will be executed periodically from the main program loop.
    // Use it to perform any periodic tasks that must occur during normal operation.
    // If your platform has a watchdog timer, it is advisable to update it here.
    // If your platform doesn't need to do anything special periodically, you can leave this function empty.
}

void __attribute__((noreturn)) sys_shutdown() {
    // This function will be called to shut down the system.
    // If your platform does not support shutting down, you can leave this function empty.
}

void __attribute__((noreturn)) sys_reboot(bool bootloader) {
    // This function will be called to reboot the system.
    // If `bootloader` is true, the system should reboot into the bootloader.
    // If `bootloader` is false, the system should reboot normally, into the main program.
    // If your platform does not support rebooting into a bootloader, you can ignore the `bootloader` parameter.
    // If your platform does not support rebooting at all, you can leave this function empty.
}

BootType sys_boot_type() {
    // This function should return the type of boot that just occurred.
    // It will always be called between `sys_boot_begin()` and `sys_boot_end()`.
    // See sys.h for the possible values of BootType.
    // If your platform does not support distinguishing between different types of boot, you can return BOOT_COLD.
}
