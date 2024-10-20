/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdbool.h>
#include <string.h>
#include "platform/adc.h"
#include "platform/defs.h"
#include "platform/flash.h"
#include "platform/helpers.h"
#include "platform/sys.h"
#include "platform/time.h"
#include "platform/types.h"
#include "platform/wifi.h"

#include "io/aahrs.h"
#include "io/esc.h"
#include "io/gps.h"
#include "io/receiver.h"
#include "io/servo.h"

#include "modes/aircraft.h"

#include "sys/boot.h"
#include "sys/configuration.h"
#include "sys/log.h"
#include "sys/print.h"
#include "sys/runtime.h"
#include "sys/version.h"

int main() {
    boot_begin();
    print("\nhello and welcome to pico-fbw v%s!\nrunning on \"%s\", HAL v%s", PICO_FBW_VERSION, PLATFORM_NAME,
          PLATFORM_VERSION);

    // Mount filesystem and load config
    boot_set_progress(0, "Mounting filesystem");
    if (lfs_mount(&lfs, &lfs_cfg) != LFS_ERR_OK) {
        // Failed to mount, try formatting
        printpre("boot", "filesystem corrupt, attempting to format...");
        lfs_format(&lfs, &lfs_cfg);
        if (lfs_mount(&lfs, &lfs_cfg) != LFS_ERR_OK)
            log_message(TYPE_FATAL, "Failed to mount filesystem!", 250, 0, true);
    }
    boot_set_progress(5, "Loading configuration");
    config_load();

    // Check version
    boot_set_progress(10, "Checking for updates");
    char version[64] = "";
    VersionCheck versionCheck = version_check(version);
    if (versionCheck != VERSION_SAME) {
        if (versionCheck == VERSION_ERROR) {
            log_message(TYPE_FATAL, "Failed to run update checker!", 250, 0, true);
        } else {
            printpre("boot", "performing a system update from v%s to v%s, please wait...",
                     (strcmp(version, "") == 0) ? "0.0.0" : version, PICO_FBW_VERSION);
            // << Insert system update code here, if applicable >>
            // Update flash with new version
            version_save();
            printpre("boot", "update done!");
        }
    } else {
        printpre("boot", "no updates required");
    }

    // Receiver
    u32 num_pins = 5; // Maximum amount is 5 pins, may be overridden
    u32 pins[num_pins];
    f32 deviations[num_pins];
    receiver_get_pins(pins, &num_pins, deviations);
    boot_set_progress(15, "Enabling receiver");
    receiver_enable(pins, num_pins);
    if (!(bool)config.general[GENERAL_SKIP_CALIBRATION]) {
        printpre("boot", "validating receiver calibration");
        ReceiverCalibrationStatus status = receiver_is_calibrated();
        switch (status) {
            case RECEIVERCALIBRATION_OK:
                break;
            case RECEIVERCALIBRATION_INVALID:
                printpre("boot", "receiver calibration was completed for a different control mode!");
                /* fall through */
            default:
            case RECEIVERCALIBRATION_INCOMPLETE:
                printpre("boot", "receiver calibration not found!");
                printpre("boot", "calibrating now...do not touch the transmitter!");
                if (!receiver_calibrate(pins, num_pins, deviations, 2000, 2, 3) || receiver_is_calibrated() != 0) {
                    log_message(TYPE_FATAL, "Receiver calibration failed!", 500, 0, true);
                } else {
                    printpre("boot", "calibration successful!");
                }
                break;
        }
    } else {
        log_message(TYPE_WARNING, "Receiver calibration skipped!", 500, 0, false);
    }

    // Servos
    boot_set_progress(25, "Enabling servos");
    u32 num_servos = 4; // Maximum is 4 servos, may be overridden
    u32 servos[num_servos];
    servo_get_pins(servos, &num_servos);
    servo_enable(servos, num_servos);
    const f32 degrees[] = DEFAULT_SERVO_TEST;
    servo_test(servos, num_servos, degrees, count_of(degrees), DEFAULT_SERVO_TEST_PAUSE_MS);

    // ESC
    if (receiver_has_athr()) {
        boot_set_progress(35, "Enabling ESC");
        esc_enable((u32)config.pins[PINS_ESC_THROTTLE]);
        if (!(bool)config.general[GENERAL_SKIP_CALIBRATION]) {
            printpre("boot", "validating throttle detent calibration");
            if (!esc_is_calibrated()) {
                printpre("boot", "throttle detent calibration not found!");
                if (!esc_calibrate((u32)config.pins[PINS_ESC_THROTTLE])) {
                    log_message(TYPE_ERROR, "Throttle detent calibration failed!", 500, 0, false);
                } else {
                    printpre("boot", "throttle detent calibration successful!");
                }
            }
        } else {
            log_message(TYPE_WARNING, "Throttle detent calibration skipped!", 500, 0, false);
        }
    }

    // Check for watchdog reboot
    if (boot_type() == BOOT_WATCHDOG) {
        log_message(TYPE_ERROR, "Watchdog rebooted!", 500, 150, true);
        print("\nPlease report this error! Only direct mode is available until the next reboot.\n");
        // Lock into direct mode for safety reasons
        // This is done now because minimum peripherals have been initialized, but not more complex ones that could be causing
        // the watchdog reboots
        sys_boot_end();
        aircraft.change_to(MODE_DIRECT);
        while (true)
            runtime_loop_minimal();
    }

    // AAHRS
    boot_set_progress(45, "Initializing AAHRS");
    if (!aahrs.init()) {
        // If AAHRS is calibrated: severity level is only an error as we could be in flight and we want to finish the boot,
        // If AAHRS is not calibrated: severity level is a fatal error to help point the user in the right direction
        LogType severity = aahrs.isCalibrated ? TYPE_ERROR : TYPE_FATAL;
        // Host platforms are the only exception, as AAHRS will always fail to initialize
#ifdef FBW_PLATFORM_HOST
        severity = TYPE_ERROR;
#endif
        log_message(severity, "AAHRS initialization failed!", 1000, 0, false);
    }
    if (!(bool)config.general[GENERAL_SKIP_CALIBRATION]) {
        printpre("boot", "validating AAHRS calibration");
        if (!aahrs.isCalibrated) {
            printpre("boot", "AAHRS calibration not found!");
            if (!aahrs.calibrate()) {
                log_message(TYPE_FATAL, "AAHRS calibration failed!", 1000, 0, true);
            } else {
                printpre("boot", "AAHRS calibration successful!");
            }
        }
    } else {
        log_message(TYPE_WARNING, "AAHRS calibration skipped!", 1000, 0, false);
    }

    // GPS
    if (gps.is_supported()) {
        while (time_ms() < 1000)
            ;
        boot_set_progress(65, "Initializing GPS");
        if (gps.init()) {
            printpre("boot", "GPS ok");
            // We don't set the GPS safe just yet, comms are good but we are still unsure if the data is good
            log_message(TYPE_INFO, "GPS has no signal.", 5000, 150, false);
        } else {
            log_message(TYPE_ERROR, "GPS not found!", 1000, 0, false);
        }
    }

    // Platform-specific feature setup

#if PLATFORM_SUPPORTS_WIFI
    boot_set_progress(85, "Initializing Wi-Fi");
    bool setup = false;
    if (lfs_mount(&wwwfs, &wwwfs_cfg) != LFS_ERR_OK)
        goto fail;
    switch ((WifiEnabled)config.general[GENERAL_WIFI_ENABLED]) {
        case WIFI_ENABLED_OPEN:
            setup = wifi_setup(config.wifi.ssid, NULL);
            break;
        case WIFI_ENABLED_PASS:
            setup = wifi_setup(config.wifi.ssid, config.wifi.pass);
        /* fall through */
        default:
            break;
    }
    if (!setup)
    fail:
        log_message(TYPE_ERROR, "Wi-Fi setup failed!", 2000, 0, false);
#endif

// ADC
#if PLATFORM_SUPPORTS_ADC
    adc_setup(ADC_PINS, ADC_NUM_CHANNELS);
#endif

    boot_set_progress(90, "Finishing up");
    // Final platform-specific setup tasks
    boot_complete();
    // Main program loop
    while (true)
        runtime_loop(true);

    return 0; // How did we get here?
}
