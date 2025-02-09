/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include "platform/defs.h"
#include "platform/sys.h"

#include "ctrl/aircraft.h"
#include "sys/boot.h"
#include "sys/configuration.h"
#include "sys/log.h"
#include "sys/print.h"
#include "sys/runtime.h"

int main() {
    boot_begin();
    print("\nhello and welcome to pico-fbw v%s!\nrunning on \"%s\", HAL v%s", PICO_FBW_VERSION, PLATFORM_NAME,
          PLATFORM_VERSION);

    boot_set_progress(0, "Mounting filesystem");
    boot_mount_fs();
    boot_set_progress(5, "Loading configuration");
    config_load();

    boot_set_progress(10, "Checking for updates");
    boot_do_updates();

    boot_set_progress(15, "Enabling receiver");
    boot_init_receiver();

    boot_set_progress(25, "Enabling servos");
    boot_init_servos();

    boot_set_progress(35, "Enabling ESCs");
    boot_init_escs();

    // Check for watchdog reboot
    if (boot_type() == BOOT_WATCHDOG) {
        log_message(TYPE_ERROR, "Watchdog rebooted!", 500, 150, true);
        print("\nPlease report this error! Only direct mode is available until the next reboot.\n");
        // Lock into direct mode for safety reasons
        // This is done now because minimum peripherals have been initialized, but not more complex ones that could be
        // causing the watchdog reboots
        sys_boot_end();
        aircraft.change_to(MODE_DIRECT);
        while (true) {
            runtime_loop_minimal();
        }
    }

    boot_set_progress(45, "Initializing AAHRS");
    boot_init_aahrs();

    boot_set_progress(65, "Initializing GPS");
    boot_init_gps();

#if PLATFORM_SUPPORTS_WIFI
    boot_set_progress(80, "Initializing Wi-Fi");
    boot_init_wifi();
#endif

#if PLATFORM_SUPPORTS_ADC
    boot_set_progress(85, "Initializing ADC");
    boot_init_adc();
#endif

    boot_set_progress(90, "Finishing up");
    boot_complete();

    // Main program loop
    while (true) {
        runtime_loop(true);
    }

    return 0; // How did we get here?
}
