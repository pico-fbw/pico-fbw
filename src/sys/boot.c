/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

// TODO: refactor, shorten functions etc.
// bring out into other files?

#include <assert.h>
#include <string.h>
#include "platform/defs.h"
#include "platform/flash.h"
#include "platform/gpio.h"
#include "platform/helpers.h"
#include "platform/stdio.h"
#include "platform/time.h"
#include "platform/wifi.h"

#include "io/esc.h"
#include "io/gps.h"
#include "io/imu.h"
#include "io/receiver.h"
#include "io/servo.h"

#include "sys/configuration.h"
#include "sys/log.h"
#include "sys/print.h"
#include "sys/runtime.h"
#include "sys/version.h"

#include "boot.h"

static bool isBooted = false;

void boot_begin() {
    sys_boot_begin();
    stdio_setup();
    bool flashOk = flash_setup();
    assert(flashOk);
    // Platform setup is now done, wait a bit for everything to complete
    sleep_ms_blocking(BOOT_WAIT_MS);
    log_init();
#ifndef NO_COLOR_OUTPUT
    // If we're using color output, reset color to default in case the previous output was colored
    printraw(COLOR_RESET);
#endif
}

void boot_mount_fs() {
    if (lfs_mount(&lfs, &lfs_cfg) == LFS_ERR_OK) {
        return;
    }
    // Failed to mount, try formatting
    printpre("boot", "filesystem corrupt, attempting to format...");
    lfs_format(&lfs, &lfs_cfg);
    if (lfs_mount(&lfs, &lfs_cfg) != LFS_ERR_OK) {
        log_message(TYPE_FATAL, "Failed to mount filesystem!", 250, 0, true);
    }
}

void boot_do_updates() {
    char version[64] = "";
    VersionCheck check = version_check(version);
    if (check == VERSION_SAME) {
        printpre("boot", "no updates required");
        return;
    }
    if (check != VERSION_ERROR) {
        printpre("boot", "performing a system update from v%s to v%s, please wait...",
                 (strcmp(version, "") == 0) ? "0.0.0" : version, PICO_FBW_VERSION);
        // No updates need special handling at the moment...so nothing to do here!
        // Write new version to flash
        version_save();
        printpre("boot", "update done!");
    } else {
        log_message(TYPE_FATAL, "Failed to run update checker!", 250, 0, true);
    }
}

void boot_init_receiver() {
    u32 numPins = MAX_RECEIVER_PINS; // Maximum, will be overridden by receiver_get_pins()
    i16 pins[numPins];
    f32 deviations[numPins];
    receiver_get_pins(pins, &numPins, deviations);
    receiver_enable(pins, numPins);
    if (receiver_is_calibrated() != RECEIVERCALIBRATION_OK) {
        log_message(TYPE_ERROR, "Receiver not calibrated!", 500, 0, false);
    }
}

void boot_init_servos() {
    u32 numServos = 4; // Maximum is 4 servos, may be overridden
    i16 servos[numServos];
    servo_get_pins(servos, &numServos);
    servo_enable(servos, numServos);
    const f32 degrees[] = DEFAULT_SERVO_TEST;
    servo_test(servos, numServos, degrees, count_of(degrees), DEFAULT_SERVO_TEST_PAUSE_MS);
}

void boot_init_escs() {
    if (!receiver_has_athr()) {
        return;
    }
    esc_enable((i16)config.pins[PINS_ESC_THROTTLE]);
}

void boot_init_imu() {
    if (imu.init()) {
        printpre("boot", "IMU ok");
        if (!imu.isCalibrated) {
            log_message(TYPE_WARNING, "IMU not calibrated!", 1000, 0, false);
        }
        return;
    }
    // If IMU is calibrated: severity level is only an error as we could be in flight and we want to finish the boot
    // If IMU is not calibrated: severity level is a fatal error to help point the user in the right direction
    LogType severity = imu.isCalibrated ? TYPE_ERROR : TYPE_FATAL;
    // Host platforms are the only exception, as IMU will always fail to initialize
#if FBW_PLATFORM_HOST
    severity = TYPE_ERROR;
#endif
    log_message(severity, "IMU initialization failed!", 1000, 0, false);
}

void boot_init_gps() {
    if (!gps.is_supported()) {
        return;
    }
    if (!gps.init()) {
        log_message(TYPE_ERROR, "GPS not found!", 1000, 0, false);
        return;
    }
    printpre("boot", "GPS ok");
    // We don't set the GPS safe just yet, comms are good but we are still unsure if the data is good
}

void boot_init_wifi() {
#if PLATFORM_SUPPORTS_WIFI
    bool setup = false;
    if (lfs_mount(&wwwfs, &wwwfs_cfg) != LFS_ERR_OK) {
        goto fail;
    }
    printsys(network, "successfully mounted wifi filesystem");
    switch ((WifiEnabled)config.general[GENERAL_WIFI_ENABLED]) {
        case WIFI_ENABLED_OPEN:
            printsys(network, "setting up open wifi with ssid \"%s\"", config.wifi.ssid);
            setup = wifi_setup(config.wifi.ssid, NULL);
            break;
        case WIFI_ENABLED_PASS:
            printsys(network, "setting up wifi with ssid \"%s\" and password \"%s\"", config.wifi.ssid,
                     config.wifi.pass);
            setup = wifi_setup(config.wifi.ssid, config.wifi.pass);
        /* fall through */
        default:
            break;
    }
    if (!setup) {
    fail:
        log_message(TYPE_ERROR, "Wi-Fi setup failed!", 2000, 0, false);
    }
#endif
}

void boot_complete() {
    printpre("boot", "%s(100%%)%s Done!", COLOR_BLUE, COLOR_RESET);
    sys_boot_end();
    isBooted = true;
}

void boot_set_progress(f32 progress, const char *message) {
    printpre("boot", "%s(%.f%%)%s %s", COLOR_BLUE, progress, COLOR_RESET, message);
}

bool boot_is_booted() {
    return isBooted;
}
