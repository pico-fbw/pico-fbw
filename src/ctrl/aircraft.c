/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include "platform/defs.h"
#include "platform/time.h"
#include "platform/wifi.h"

#include "io/gps.h"
#include "io/imu.h"
#include "io/receiver.h"
#include "io/servo.h"
#include "modes/auto.h"
#include "modes/direct.h"
#include "modes/hold.h"
#include "modes/launch.h"
#include "modes/normal.h"
#include "modes/tune.h"
#include "sys/configuration.h"
#include "sys/flightplan.h"
#include "sys/log.h"
#include "sys/print.h"

#include "aircraft.h"

// Shorthand for checking GPS feature support and data validity
#define GPS_OK() (gps.is_supported() && gpsSafe)
// Speed threshold to determine if the aircraft is flying (kts)
#define SPEED_FLYING_THRESHOLD 5
// The highest amount of time that the aircraft can still be considered flying after the last control input (s)
#define STILL_FLYING_TIMEOUT 15

static Mode mode = MODE_DIRECT;
static bool isFlying = false;
static bool imuSafe = false, gpsSafe = false;
#if PLATFORM_SUPPORTS_WIFI
static bool wifiDeinitialized = false;
#endif
static Timestamp lastNonzeroInput; // Last time a control input was detected

/**
 * Determines whether or not the aircraft is currently flying.
 * @return true if the aircraft is assumed to be flying, false if it is likely grounded
 * @note Must be called somewhat frequently to ensure accuracy.
 */
static bool is_flying() {
    if (GPS_OK()) {
        // We have GPS data, speed is a good infdication of whether or not we're flying
        return gps.speed >= SPEED_FLYING_THRESHOLD;
    } else {
        // Have there been any recent control inputs?
        if (USER_INPUTTING()) {
            lastNonzeroInput = timestamp_now();
        }
        return time_since_s(&lastNonzeroInput) < STILL_FLYING_TIMEOUT;
    }
}

/**
 * Deinitializes the given mode.
 * @param mode the mode to deinitialize
 */
static void deinit_mode(Mode mode) {
    switch (mode) {
        case MODE_NORMAL:
            normal_deinit();
            break;
        case MODE_TUNE:
            tune_deinit();
            break;
        default:
            break;
    }
    mode = MODE_INVALID;
}

/**
 * Helper function to handle launch assist initialization.
 * @param next_mode the mode to transition to after launch
 * @return true if launch was initiated
 */
static bool try_launch_assist(Mode next_mode) {
    if (!(bool)config.general[GENERAL_LAUNCHASSIST_ENABLED]) {
        return false;
    }
    printsys(aircraft, "initiating launch assist");
    launch_init(next_mode);
    mode = MODE_LAUNCH;
    return true;
}

/**
 * Helper function to handle autotune initialization.
 * @return true if autotune was initiated
 */
static bool try_autotune() {
    if (tune_is_tuned() || !(bool)config.general[GENERAL_AUTOTUNE_ENABLED]) {
        return false;
    }
    printsys(aircraft, "initiating autotune");
    tune_init();
    mode = MODE_TUNE;
    return true;
}

/* Mode initializers */

static void init_normal_mode() {
    // Attempt to use launch assist before engaging normal mode
    if (try_launch_assist(MODE_NORMAL)) {
        return;
    }
    normal_init();
    mode = MODE_NORMAL;
}

static bool init_auto_mode() {
    // Check if autotune is needed
    if (try_autotune()) {
        return true;
    }
    // GPS is required for auto mode
    if (!GPS_OK()) {
        printsys(aircraft, "WARNING: GPS is required for auto mode, falling back to normal mode");
        init_normal_mode();
        return false;
    }
    if (try_launch_assist(MODE_AUTO)) {
        return true;
    }
    if (!auto_init()) {
        printsys(aircraft, "WARNING: failed to initialize auto mode, falling back to normal mode");
        init_normal_mode();
        return false;
    }
    mode = MODE_AUTO;
    return true;
}

static void init_tune_mode() {
    if (tune_is_tuned()) {
        printsys(aircraft, "already tuned, falling back to normal mode");
        init_normal_mode();
        return;
    }
    tune_init();
    mode = MODE_TUNE;
}

static void init_hold_mode() {
    // GPS is required for hold mode
    if (!GPS_OK()) {
        printsys(aircraft, "WARNING: GPS is required for hold mode, falling back to normal mode");
        init_normal_mode();
        return;
    }
    if (!hold_init()) {
        printsys(aircraft, "WARNING: failed to initialize hold mode, falling back to normal mode");
        init_normal_mode();
        return;
    }
    mode = MODE_HOLD;
}

/**
 * Initializes the given mode.
 * @param mode the mode to initialize
 */
static void init_mode(Mode mode) {
    switch (mode) {
        case MODE_DIRECT:
            mode = MODE_DIRECT;
            break;
        case MODE_LAUNCH:
            launch_init(mode);
            mode = MODE_LAUNCH;
            break;
        case MODE_NORMAL:
            init_normal_mode();
            break;
        case MODE_AUTO:
            init_auto_mode();
            break;
        case MODE_TUNE:
            init_tune_mode();
            break;
        case MODE_HOLD:
            init_hold_mode();
            break;
        default:
            mode = MODE_DIRECT;
            break;
    }
}

/**
 * Updates the current mode's logic.
 */
static void update_mode() {
    switch (mode) {
        case MODE_DIRECT:
            direct_update();
            break;
        case MODE_LAUNCH:
            launch_update();
            break;
        case MODE_NORMAL:
            normal_update();
            break;
        case MODE_AUTO:
            auto_update();
            break;
        case MODE_TUNE:
            tune_update();
            break;
        case MODE_HOLD:
            hold_update();
            break;
        default:
            direct_update();
            break;
    }
}

/**
 * Handles wifi deinitialization after taking flight.
 */
static void handle_wifi_deinit() {
#if PLATFORM_SUPPORTS_WIFI
    if (!wifiDeinitialized && isFlying) {
        // We're now airborne, so wifi is no longer needed
        if (!wifi_disable()) {
            printsys(network, "WARNING: failed to disable wifi!");
        }
        printsys(network, "wifi disabled");
        wifiDeinitialized = true;
    }
#endif
}

const char *mode_to_string(Mode mode) {
    switch (mode) {
        case MODE_LAUNCH:
            return "launch";
        case MODE_DIRECT:
            return "direct";
        case MODE_NORMAL:
            return "normal";
        case MODE_AUTO:
            return "auto";
        case MODE_TUNE:
            return "tune";
        case MODE_HOLD:
            return "hold";
        default:
            return "invalid";
    }
}

void aircraft_update() {
    update_mode();
    isFlying = is_flying();
}

void aircraft_change_mode(Mode new_mode) {
    // Deinit the current mode
    printsys(aircraft, "exiting %s mode", mode_to_string(mode));
    deinit_mode(mode);
    // All modes (except for direct) require IMU so make sure that's all good
    if (!imuSafe && new_mode != MODE_DIRECT) {
        printsys(aircraft, "IMU has failed, entering direct mode!");
        log_message(TYPE_ERROR, "IMU has failed!", 250, 0, true);
        mode = MODE_DIRECT;
        return;
    }
    // Init the new mode
    printsys(aircraft, "trying to enter %s mode", mode_to_string(new_mode));
    init_mode(new_mode);
    printsys(aircraft, "entered %s mode", mode_to_string(mode));
    handle_wifi_deinit();
}

void aircraft_set_imu_safe(bool is_safe) {
    if (is_safe == imuSafe) {
        return; // Nothing to do
    }

    imuSafe = is_safe;
    if (!imuSafe) {
        // Change to direct mode as it doesn't require IMU, and deinit
        aircraft_change_mode(MODE_DIRECT);
        imu.deinit();
        printsys(aircraft, "IMU set as unsafe");
        return;
    }
    // Last-ditch attempt to re-init IMU if it's not already
    if (!imu.ready && !imu.init()) {
        log_message(TYPE_ERROR, "IMU initialization failed!", 1000, 0, false);
        aircraft_change_mode(MODE_DIRECT);
        return;
    }
    printsys(aircraft, "IMU set as safe");
}

void aircraft_set_gps_safe(bool is_safe) {
    if (is_safe == gpsSafe) {
        return;
    }

    gpsSafe = is_safe;
    if (gpsSafe) {
        printsys(aircraft, "GPS set as safe");
        log_clear(TYPE_INFO);
        return;
    }
    printsys(aircraft, "GPS set as unsafe");
    // Return to normal mode if GPS is deemed unsafe in Auto or Hold modes (require GPS)
    if (mode == MODE_AUTO || mode == MODE_HOLD) {
        aircraft_change_mode(MODE_NORMAL);
    }
}

Mode aircraft_get_mode() {
    return mode;
}

bool aircraft_is_flying() {
    return isFlying;
}

bool aircraft_is_imu_safe() {
    return imuSafe;
}

bool aircraft_is_gps_safe() {
    return gpsSafe;
}

#if PLATFORM_SUPPORTS_WIFI
bool aircraft_wifi_deinitialized() {
    return wifiDeinitialized;
}
#endif
