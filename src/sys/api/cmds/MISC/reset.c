/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/sys.h"

#include "sys/configuration.h"
#include "sys/print.h"
#include "sys/runtime.h"

#include "reset.h"

void api_reset(const char *cmd, const char *args) {
    printraw("This will erase ALL user data stored on the device!\nReset will occur in 10 seconds...power off the device to "
             "cancel.\n");
    runtime_sleep_ms(10000, false);
    config_reset();
    printraw("Reset complete. Shutting down...\n");
    sys_shutdown();
}
