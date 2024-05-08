/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <assert.h>
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
    bool flashOk = flash_setup();
    assert(flashOk);
    log_init();
    display_init();
    sleep_ms_blocking(BOOT_WAIT_MS); // Wait for peripherals to power up
#if !NO_COLOR_OUTPUT
    printraw(COLOR_RESET); // If we're using color output, reset color to default in case the previous output was colored
#endif
}

void boot_set_progress(f32 progress, const char *message) {
    printpre("boot", "%s(%.f%%)%s %s", COLOR_BLUE, progress, COLOR_RESET, message);
    display_string(message, (i32)progress);
}

void boot_complete() {
    printpre("boot", "%s(100%%)%s Done!", COLOR_BLUE, COLOR_RESET);
    if (log_count_errs() == 0)
        display_anim();
    sys_boot_end();
    isBooted = true;
}

bool boot_is_booted() {
    return isBooted;
}
