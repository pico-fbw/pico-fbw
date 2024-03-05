/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/defs.h"
#include "platform/flash.h"
#include "platform/gpio.h"
#include "platform/stdio.h"
#include "platform/time.h"

#include "io/display.h"

#include "sys/configuration.h"
#include "sys/log.h"
#include "sys/print.h"
#include "sys/runtime.h"

#include "boot.h"

bool isBooted = false;

void boot_begin() {
    sys_boot_begin();
    stdio_setup();
    flash_setup();
    log_init();
    isBooted = false;
#ifdef PIN_FBW
    gpio_setup(PIN_FBW, INPUT_PULLDOWN);
    if (runtime_is_fbw())
        display_init();
#endif
    sleep_ms_blocking(BOOT_WAIT_MS); // Wait for peripherals to power up
}

void boot_set_progress(float progress, const char *message) {
    print("[boot] (%1.f%%) %s", progress, message);
    if (runtime_is_fbw())
        display_string(message, (i32)progress);
}

void boot_complete() {
    print("[boot] (100%%) Done!");
    sys_boot_end();
    if (runtime_is_fbw() && log_count_errs() == 0)
        display_anim();
    isBooted = true;
}

bool boot_is_booted() {
    return isBooted;
}
