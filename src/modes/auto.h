#pragma once

#include <stdbool.h>
#include "platform/int.h"

/* PID constants for the autopilot's lateral guidance. */
#define latGuid_kP 0.005
#define latGuid_kI 0.008
#define latGuid_kD 0.002
#define latGuid_tau 0.001
#define latGuid_lim 33 // The maximum roll angle the autopilot can command
#define latGuid_integLim 50.0

/* PID constants for the autopilot's vertical guidance. */
#define vertGuid_kP 0.05
#define vertGuid_kI 0.0025
#define vertGuid_kD 0.001
#define vertGuid_tau 0.001
#define vertGuid_loLim -15 // The minimum pitch angle the autopilot can command
#define vertGuid_hiLim 25  // The maximum pitch angle the autopilot can command
#define vertGuid_integLim 50.0

typedef struct Waypoint {
    long double lat, lng;
    i32 alt;
    float speed;
    i32 drop;
} Waypoint;
#define WAYPOINT_NUM_FIELDS 5

typedef enum BayPosition {
    CLOSED,
    OPEN,
} BayPosition;

/**
 * Initializes auto mode.
 * @return true if initialization was successful
 */
bool auto_init();

/**
 * Executes one cycle of auto mode.
 */
void auto_update();

/**
 * Manually sets a temporary Waypoint target.
 * @param wpt the Waypoint to target
 * @param callback callback function to call when the target is reached, or NULL if not needed
 */
void auto_set(Waypoint wpt, void (*callback)(void));

/**
 * Sets the position of the drop bay (servo) mechanism.
 * @param pos the position to set the drop bay mechanism to
 */
void auto_set_bay_position(BayPosition pos);
