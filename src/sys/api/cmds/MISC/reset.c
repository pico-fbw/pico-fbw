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

static inline bool reset_callback(struct repeating_timer *t) {
    char *ans = stdin_read_line();
    if (ans != NULL) {
        if (strcasecmp(ans, "Y") == 0) {
            printf("Reset will begin shortly...\n");
            sleep_ms(2500);
            flash_reset();
            printf("Reset complete. Shutting down...\n");
            platform_shutdown();
        } else {
            printf("Reset cancelled.\n");
        }
        return false;
    }
    return true;
}

uint api_reset(const char *cmd, const char *args) {
    if (strncasecmp(args, "-F", 2) != 0) {
        printf("This will erase ALL data stored on the device! Are you sure? (y/n)\n");
        // Create timer to avoid making an infinite loop
        static struct repeating_timer callback;
        add_repeating_timer_ms(50, reset_callback, NULL, &callback);
    }
}
