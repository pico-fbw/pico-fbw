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

typedef enum FlightplanError {
    FLIGHTPLAN_STATUS_OK,
    FLIGHTPLAN_STATUS_AWAITING,
    FLIGHTPLAN_STATUS_GPS_OFFSET,
    FLIGHTPLAN_WARN_FW_VERSION,
    FLIGHTPLAN_ERR_PARSE,
    FLIGHTPLAN_ERR_VERSION,
    FLIGHTPLAN_ERR_ALREADY_PARSED,
    FLIGHTPLAN_ERR_MEM,
} FlightplanError;

/**
 * @return whether the given waypoint contains valid data
 */
bool waypoint_is_valid(Waypoint *wpt);

/**
 * @return true if a Flightplan has previously been parsed
 */
bool flightplan_was_parsed();

/**
 * Parses a Flightplan from a JSON string.
 * @param json the JSON string to parse
 * @param silent if true, suppresses log messages
 * @return the result of the parse attempt, if successful,
 * the parsed Flightplan can be retrieved with `flightplan_get()`
 */
FlightplanError flightplan_parse(const char *json, bool silent);

/**
 * @return the parsed Flightplan, or NULL if no Flightplan has been parsed
 */
Flightplan *flightplan_get();

/**
 * @return the current state of the Flightplan
 */
FlightplanError flightplan_state();
