#pragma once

#include <stdbool.h>
#include "platform/defs.h"

#define MODE_MIN MODE_DIRECT
// clang-format off
typedef enum Mode {
    MODE_INVALID,
    MODE_DIRECT,
    MODE_LAUNCH,
    MODE_NORMAL,
    MODE_AUTO,
    MODE_TUNE,
    MODE_HOLD,
} Mode;
// clang-format on
#define MODE_MAX MODE_HOLD

// Helper macros to determine if the user is currently inputting on the controls
// If used, ensure to #include "io/receiver.h" and "sys/configuration.h"
#define DEADBAND config.control[CONTROL_DEADBAND]
#define ROLL_INPUT() (fabsf(receiver_get((i16)config.pins[PINS_INPUT_AIL], RECEIVER_MODE_DEGREE) - 90.f) > DEADBAND)
#define PITCH_INPUT() (fabsf(receiver_get((i16)config.pins[PINS_INPUT_ELE], RECEIVER_MODE_DEGREE) - 90.f) > DEADBAND)
#define YAW_INPUT()                                                                                                    \
    (receiver_has_rud() &&                                                                                             \
     fabsf(receiver_get((i16)config.pins[PINS_INPUT_RUD], RECEIVER_MODE_DEGREE) - 90.f) > DEADBAND)
// Throttle input usually isn't self-centering so it's more difficult to determine if there is input
#define USER_INPUTTING() (ROLL_INPUT() || PITCH_INPUT() || YAW_INPUT())

/**
 * @param mode mode to convert to string
 * @return string representation of the mode
 */
const char *mode_to_string(Mode mode);

/**
 * Runs the code of the system's currently selected mode.
 * Updates the aircraft state and mode logic.
 */
void aircraft_update();

/**
 * Transitions the aircraft to a specified mode.
 * @param mode mode to transition to
 */
void aircraft_change_mode(Mode mode);

/**
 * Sets whether the IMU data is safe to use.
 * @param state whether or not the IMU data is safe to use
 */
void aircraft_set_imu_safe(bool state);

/**
 * Sets whether the GPS data is safe to use.
 * @param state whether or not the GPS data is safe to use
 */
void aircraft_set_gps_safe(bool state);

/**
 * Gets the current aircraft mode.
 * @return the current mode
 */
Mode aircraft_get_mode();

/**
 * Gets whether the aircraft is currently flying.
 * @return true if the aircraft is flying, false otherwise
 */
bool aircraft_is_flying();

/**
 * Gets whether the IMU data is safe to use.
 * @return true if the IMU is safe, false otherwise
 */
bool aircraft_is_imu_safe();

/**
 * Gets whether the GPS data is safe to use.
 * @return true if the GPS is safe, false otherwise
 */
bool aircraft_is_gps_safe();

#if PLATFORM_SUPPORTS_WIFI
/**
 * Gets whether wifi has been deinitialized after taking flight.
 * @return true if wifi has been deinitialized, false otherwise
 */
bool aircraft_wifi_deinitialized();
#endif
