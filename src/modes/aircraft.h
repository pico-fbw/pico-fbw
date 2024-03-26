#pragma once

#include <stdbool.h>

#define MODE_MIN MODE_DIRECT
// clang-format off
typedef enum Mode {
    MODE_DIRECT,
    MODE_NORMAL,
    MODE_AUTO,
    MODE_TUNE,
    MODE_HOLD,
} Mode;
// clang-format on
#define MODE_MAX MODE_HOLD

typedef void (*aircraft_update_t)();
typedef void (*aircraft_change_to_t)(Mode);
typedef void (*aircraft_set_aahrs_safe_t)(bool);
typedef void (*aircraft_set_gps_safe_t)(bool);

typedef struct Aircraft {
    Mode mode;      // (Read-only)
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
