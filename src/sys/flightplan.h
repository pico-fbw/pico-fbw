#pragma once

#include <stdbool.h>
#include "platform/types.h"

#include "modes/auto.h"

#define FLIGHTPLAN_MSG_STATUS_GPS_OFFSET "When ready, please engage auto mode to calibrate the GPS."
#define FLIGHTPLAN_MSG_WARN_FW_VERSION "A new firmware version is available!"

typedef struct Flightplan {
    char *version;
    char *version_fw;
    i32 alt_samples;
    Waypoint *waypoints;
    u32 waypoint_count;

    char *json;
} Flightplan;

typedef enum FlightplanState {
    FLIGHTPLAN_STATUS_OK,
    FLIGHTPLAN_STATUS_AWAITING,
    FLIGHTPLAN_STATUS_GPS_OFFSET,
    FLIGHTPLAN_WARN_FW_VERSION,
    FLIGHTPLAN_ERR_PARSE,
    FLIGHTPLAN_ERR_VERSION,
    FLIGHTPLAN_ERR_MEM,
} FlightplanState;

/**
 * @return whether the given waypoint contains valid data
 */
bool waypoint_is_valid(Waypoint *wpt);

/**
 * Parses a Flightplan from a JSON string.
 * @param json the JSON string to parse
 * @param flightplan the Flightplan to populate
 * @param silent if true, suppresses log messages
 * @return the result of the parse attempt, if successful,
 */
FlightplanState flightplan_parse(const char *json, Flightplan *flightplan, bool silent);

/**
 * @return the active Flightplan, or NULL there is none
 */
Flightplan *flightplan_get();

/**
 * Sets the active Flightplan.
 * @param flightplan the Flightplan to set
 */
void flightplan_set(Flightplan flightplan);
