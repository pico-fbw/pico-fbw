/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "pico/config.h"
#include "pico/platform.h"
#include "pico/stdio.h"
#include "pico/time.h"
#include "pico/types.h"

#include "hardware/watchdog.h"
#ifdef RASPBERRYPI_PICO_W
    #include "pico/cyw43_arch.h"
#endif

#include "io/aahrs.h"
#include "io/esc.h"
#include "io/flash.h"
#include "io/gps.h"
#include "io/platform.h"
#include "io/pwm.h"
#include "io/servo.h"
#include "modes/modes.h"
#include "sys/config.h"
#include "sys/info.h"
#include "sys/log.h"
#include "wifly/wifly.h"

int main() {
    // Establish comms and initialize critical systems
    platform_boot_begin();
    if (stdio_init_all()) sleep_ms(BOOT_WAIT_MS);
    #if defined(RASPBERRYPI_PICO)
        printf("\nhello and welcome to pico-fbw v%s!\n", PICO_FBW_VERSION);
    #elif defined(RASPBERRYPI_PICO_W)
        printf("\nhello and welcome to pico(w)-fbw v%s!\n", PICO_FBW_VERSION);
        platform_boot_setProgress(10, "Initializing CYW43 architecture");
        cyw43_arch_init();
    #endif
    log_init();

    // Load flash into RAM
    if (platform_buttonPressed()) {
        // If the button is being held down, erase flash and shut down if it continues to be held down for 3s
        flash_erase();
        printf("[boot] flash erased");
        platform_shutdown();
    }
    platform_boot_setProgress(20, "Loading flash");
    printf("[flash] loaded %d bytes\n", flash_load());

    // Check version
    platform_boot_setProgress(30, "Checking for updates");
    const char *version = flash.version;
    int versionCheck = info_checkVersion(version);
    if (versionCheck != 0) {
        if (versionCheck < -2) {
            log_message(FATAL, "Failed to run update checker!", 250, 0, true);
        } else {
            printf("[update] performing a system update from v%s to v%s, please wait...\n", (strcmp(version, "") == 0) ? "0.0.0" : version, PICO_FBW_VERSION);
            // << Insert system update code here, if applicable >>
            // Update flash with new version
            strcpy(flash.version, PICO_FBW_VERSION);
            flash_save();
            printf("[update] done!\n");
        }
    } else {
        printf("[update] no updates required\n");
    }

    // PWM (in)
    uint num_pins = 5; // Maximum amount is 5 pins, may be overridden
    uint pins[num_pins];
    float deviations[num_pins];
    pwm_getPins(pins, &num_pins, deviations);
    platform_boot_setProgress(35, "Enabling PWM");
    pwm_enable(pins, num_pins);
    if (!(bool)flash.general[GENERAL_SKIP_CALIBRATION]) {
        if (print.fbw) printf("[boot] validating PWM calibration\n");
        PWMCalibrationStatus calibrationResult = pwm_isCalibrated();
        switch (calibrationResult) {
            case PWMCALIBRATION_OK:
                break;
            case PWMCALIBRATION_INVALID:
                if (print.fbw) printf("[boot] PWM calibration was completed for a different control mode!\n");
            default:
            case PWMCALIBRATION_INCOMPLETE:
                if (print.fbw) printf("[boot] PWM calibration not found!\n");
                if (print.fbw) printf("[boot] calibrating now...do not touch the transmitter!\n");
                if (!pwm_calibrate(pins, num_pins, deviations, 2000, 2, 3) || pwm_isCalibrated() != 0) {
                    log_message(FATAL, "PWM calibration failed!", 500, 0, true);
                } else {
                    if (print.fbw) printf("[boot] calibration successful!\n");
                }
                break;
        }
    } else {
        log_message(WARNING, "PWM calibration skipped!", 500, 0, false);
    }

    // Servos
    platform_boot_setProgress(45, "Enabling servos");
    uint num_servos = 4; // Maximum is 4 servos, may be overridden
    uint servos[num_servos];
    servo_getPins(servos, &num_servos);
    for (uint8_t s = 0; s < num_servos; s++) {
        if (servo_enable(servos[s]) != 0) {
            if (print.fbw) printf("[boot] failed to initialize servo #%d\n", s);
            log_message(FATAL, "Failed to initialize a servo!", 800, 0, false);
        }
    }
    const uint16_t degrees[] = DEFAULT_SERVO_TEST;
    servo_test(servos, num_servos, degrees, NUM_DEFAULT_SERVO_TEST, DEFAULT_SERVO_TEST_PAUSE_MS);

    // ESC
    if (pwm_hasAthr()) {
        platform_boot_setProgress(60, "Enabling ESC");
        if (esc_enable((uint)flash.pins[PINS_ESC_THROTTLE]) != 0) {
            log_message(FATAL, "Failed to initialize an ESC!", 800, 0, false);
        }
        if (!(bool)flash.general[GENERAL_SKIP_CALIBRATION]) {
            if (print.fbw) printf("[boot] validating throttle detent calibration\n");
            if (!esc_isCalibrated()) {
                if (print.fbw) printf("[boot] throttle detent calibration not found!\n");
                if (!esc_calibrate((uint)flash.pins[PINS_ESC_THROTTLE])) {
                    log_message(ERROR, "Throttle detent calibration failed!", 800, 0, false);
                } else {
                    if (print.fbw) printf("[boot] throttle detent calibration successful!\n");
                }
            }
        } else {
            log_message(WARNING, "Throttle detent calibration skipped!", 800, 0, false);
        }
    }

    // AAHRS
    platform_boot_setProgress(65, "Initializing AAHRS");
    if (!aahrs.init()) {
        // If AAHRS is calibrated only throw an error as we could be in flight and we want to finish the boot,
        // but if it's not calibrated we shouldn't be in flight, thus we should throw a fatal error so calibration does not commence
        log_message(aahrs.isCalibrated ? ERROR : FATAL, "AAHRS initialization failed!", 1000, 0, false);
    }
    if (!(bool)flash.general[GENERAL_SKIP_CALIBRATION]) {
        if (print.fbw) printf("[boot] validating AAHRS calibration\n");
        if (!aahrs.isCalibrated) {
            if (print.fbw) printf("[boot] AAHRS calibration not found!\n");
            if (!aahrs.calibrate()) {
                log_message(FATAL, "AAHRS calibration failed!", 1000, 0, true);
            } else {
                if (print.fbw) printf("[boot] AAHRS calibration successful!\n");
            }
        }
    } else {
        log_message(WARNING, "AAHRS calibration skipped!", 1000, 0, false);
    }

    // GPS
    if (gps.isSupported()) {
        while (time_us_64() < (1000 * 1000));
        platform_boot_setProgress(80, "Initializing GPS");
        if (gps.init()) {
            if (print.fbw) printf("[boot] GPS ok\n");
            // We don't set the GPS safe just yet, communications have been established but we are still unsure if the data is okay
            log_message(LOG, "GPS has no signal.", 2000, 0, false);
        } else {
            log_message(ERROR, "GPS not found!", 2000, 0, false);
        }
    }

    // Watchdog
    platform_boot_setProgress(90, "Enabling watchdog");
    if (platform_boot_type() == BOOT_WATCHDOG) {
        log_message(ERROR, "Watchdog rebooted!", 500, 150, true);
        if (print.fbw) printf("\nPlease report this error! Only direct mode is available until the next reboot.\n\n");
        platform_boot_complete();
        aircraft.changeTo(MODE_DIRECT);
        while (true) {
            aircraft.update();
            watchdog_update();
        }
    }
    platform_enable_watchdog();

    // Wi-Fly
    #ifdef RASPBERRYPI_PICO_W
        if ((WiflyEnableStatus)flash.general[GENERAL_WIFLY_STATUS] != WIFLY_DISABLED) {
            platform_boot_setProgress(95, "Initializing Wi-Fly");
            wifly_init();  
        }
    #endif

    platform_boot_setProgress(100, "Done!");
    platform_boot_complete();
    // Main program loop:
    while (true) {
        platform_loop(true);
    }

    return 0; // How did we get here?
}
