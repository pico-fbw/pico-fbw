#pragma once

#include <stdbool.h>
#include "platform/types.h"

// TODO: finish tuning these

/* PID constants for the autopilot's lateral guidance. */
#define LATGD_KP 0.1
#define LATGD_KI 0
#define LATGD_KD 0
#define LATGD_TAU 1
#define LATGD_LIM 15 // The maximum roll angle the autopilot can command

/* PID constants for the autopilot's vertical guidance. */
#define VERTGD_KP 0.1
#define VERTGD_KI 0
#define VERTGD_KD 0
#define VERTGD_TAU 1
#define VERTGD_LIM_MIN -5 // The minimum pitch angle the autopilot can command
#define VERTGD_LIM_MAX 10 // The maximum pitch angle the autopilot can command

typedef struct Waypoint {
    f64 lat, lng;
    i32 alt;
    f32 speed;
    i32 drop;
} Waypoint;
#define WAYPOINT_NUM_FIELDS 5

typedef enum BayPosition {
    POS_INVALID,
    POS_CLOSED,
    POS_OPEN,
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
