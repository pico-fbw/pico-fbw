/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/sys.h"

void sys_boot_begin() {
}

void sys_boot_end() {
}

void sys_periodic() {
}

void __attribute__((noreturn)) sys_shutdown() {
}

void __attribute__((noreturn)) sys_reboot(bool bootloader) {
}

BootType sys_boot_type() {
}
