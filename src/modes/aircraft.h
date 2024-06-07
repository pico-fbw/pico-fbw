#pragma once

#include <stdbool.h>

#define MODE_MIN MODE_DIRECT
// clang-format off
typedef enum Mode {
    MODE_INVALID,
    MODE_LAUNCH,
    MODE_DIRECT,
    MODE_NORMAL,
    MODE_AUTO,
    MODE_TUNE,
    MODE_HOLD,
} Mode;
// clang-format on
#define MODE_MAX MODE_HOLD

// Shorthand for checking GPS feature support and data validitity
#define GPS_OK() (gps.is_supported() && aircraft.gpsSafe)

// Helper macros to determine if the user is currently inputting on the controls
// If used, ensure to #include "io/receiver.h" and "sys/configuration.h"
#define DEADBAND config.control[CONTROL_DEADBAND]
#define ROLL_INPUT() (fabsf(receiver_get((u32)config.pins[PINS_INPUT_AIL], RECEIVER_MODE_DEGREE) - 90.f) > DEADBAND)
#define PITCH_INPUT() (fabsf(receiver_get((u32)config.pins[PINS_INPUT_ELE], RECEIVER_MODE_DEGREE) - 90.f) > DEADBAND)
#define YAW_INPUT()                                                                                                            \
    (receiver_has_rud() && fabsf(receiver_get((u32)config.pins[PINS_INPUT_RUD], RECEIVER_MODE_DEGREE) - 90.f) > DEADBAND)
#define THROTTLE_INPUT() (fabsf(receiver_get((u32)config.pins[PINS_INPUT_THROTTLE], RECEIVER_MODE_PERCENT)) > DEADBAND)
#define USER_INPUTTING() (ROLL_INPUT() || PITCH_INPUT() || YAW_INPUT() || THROTTLE_INPUT())

typedef void (*aircraft_update_t)();
typedef void (*aircraft_change_to_t)(Mode);
typedef void (*aircraft_set_aahrs_safe_t)(bool);
typedef void (*aircraft_set_gps_safe_t)(bool);

typedef struct Aircraft {
    Mode mode;      // (Read-only)
    bool isFlying;  // (Read-only)
    bool aahrsSafe; // (Read-only)
    bool gpsSafe;   // (Read-only)
    /**
     * Runs the code of the system's currently selected mode.
     */
    aircraft_update_t update;
    /**
     * Transitions the aircraft to a specified mode.
     * @param mode The mode to transition to.
     */
    aircraft_change_to_t change_to;
    /**
     * @param state Declares whether or not the AAHRS data is safe to use.
     */
    aircraft_set_aahrs_safe_t set_aahrs_safe;
    /**
     * @param state Declares whether or not the GPS data is safe to use.
     */
    aircraft_set_gps_safe_t set_gps_safe;
} Aircraft;

extern Aircraft aircraft;
