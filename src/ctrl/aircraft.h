#pragma once

#include <stdbool.h>
#include "platform/defs.h"

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
#define YAW_INPUT()                                                                                                    \
    (receiver_has_rud() &&                                                                                             \
     fabsf(receiver_get((u32)config.pins[PINS_INPUT_RUD], RECEIVER_MODE_DEGREE) - 90.f) > DEADBAND)
// Throttle input usually isn't self-centering so it's more difficult to determine if there is input
#define USER_INPUTTING() (ROLL_INPUT() || PITCH_INPUT() || YAW_INPUT())

typedef struct Aircraft {
    Mode mode;     // (Read-only)
    bool isFlying; // (Read-only)
#if PLATFORM_SUPPORTS_WIFI
    bool wifiDeinitialized; // (Read-only)
#endif
    bool aahrsSafe; // (Read-only)
    bool gpsSafe;   // (Read-only)
    /**
     * Runs the code of the system's currently selected mode.
     */
    void (*update)();
    /**
     * Transitions the aircraft to a specified mode.
     * @param mode The mode to transition to.
     */
    void (*change_to)(Mode mode);
    /**
     * @param state Declares whether or not the AAHRS data is safe to use.
     */
    void (*set_aahrs_safe)(bool state);
    /**
     * @param state Declares whether or not the GPS data is safe to use.
     */
    void (*set_gps_safe)(bool state);
} Aircraft;

extern Aircraft aircraft;
