#pragma once

#include <stdbool.h>
#include "platform/int.h"

#include "modes/auto.h"

typedef struct Flightplan {
    char *version;
    char *version_fw;
    i32 alt_samples;
    Waypoint *waypoints;
    u32 waypoint_count;

    char *json;
} Flightplan;

typedef enum FlightplanError {
    FPLAN_STATUS_OK,
    FPLAN_STATUS_AWAITING,
    FPLAN_STATUS_GPS_OFFSET,
    FPLAN_WARN_FW_VERSION,
    FPLAN_ERR_PARSE,
    FPLAN_ERR_VERSION,
    FPLAN_ERR_MEM,
} FlightplanError;

/**
 * Parses a Flightplan from a JSON string.
 * @param json the JSON string to parse
 * @return the result of the parse attempt, if successful,
 * the parsed Flightplan can be retrieved with `flightplan_get()`
 */
FlightplanError flightplan_parse(const char *json);

/**
 * @return true if a Flightplan has previously been parsed
 */
bool flightplan_was_parsed();

/**
 * @return the parsed Flightplan, or NULL if no Flightplan has been parsed
 */
Flightplan *flightplan_get();

/**
 * @return the current state of the Flightplan
 */
FlightplanError flightplan_state();
