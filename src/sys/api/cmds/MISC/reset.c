/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include <string.h>
#include "pico/time.h"
#include "pico/types.h"

#include "hardware/watchdog.h"

#include "../../../../io/flash.h"
#include "../../../../io/serial.h"
#include "../../../../io/platform.h"

#include "reset.h"

void api_reset(const char *cmd, const char *args) {
    printf("This will erase ALL user data stored on the device!\nReset will occur in 10 seconds...power off the device to cancel.\n");
    platform_sleep_ms(10000, false);
    flash_erase();
    printf("Reset complete. Shutting down...\n");
    platform_shutdown();
}
