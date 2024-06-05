/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/defs.h"
#include "platform/gpio.h"
#include "platform/sys.h"

#include "sys/configuration.h"
#include "sys/print.h"
#include "sys/runtime.h"

#include "reset.h"

i32 api_reset(const char *args) {
    printraw("This will erase ALL user data stored on the device!\nReset will occur in 10 seconds...power off the device to "
             "cancel.\n");
    runtime_sleep_ms(10000, false);
    config_reset();
    printraw("Reset complete. Shutting down...\n");
#ifdef PIN_LED
    gpio_set(PIN_LED, STATE_LOW);
#endif
    sys_shutdown();
    return -1;
    (void)args;
}
