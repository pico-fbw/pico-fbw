#pragma once

#include <stdbool.h>
#include "platform/types.h"

#include "modes/auto.h"

#define FLIGHTPLAN_MSG_STATUS_GPS_OFFSET "GPS calibration is required before flight."
#define FLIGHTPLAN_MSG_WARN_FW_VERSION "A new firmware version is available!"

typedef struct Flightplan {
    char *version;
    char *version_fw;
    i32 alt_samples;
    Waypoint *waypoints;
    u32 waypoint_count;
    // Metadata
    char *name; // Name of the JSON file this Flightplan was parsed from
    char *json;
} Flightplan;

typedef enum FlightplanState {
    FLIGHTPLAN_STATUS_OK,
    FLIGHTPLAN_STATUS_AWAITING,
    FLIGHTPLAN_STATUS_GPS_OFFSET,
    FLIGHTPLAN_WARN_FW_VERSION,
    FLIGHTPLAN_ERR_LOAD,
    FLIGHTPLAN_ERR_PARSE,
    FLIGHTPLAN_ERR_VERSION,
    FLIGHTPLAN_ERR_MEM,
} FlightplanState;

/**
 * @return whether the given waypoint contains valid data
 */
bool waypoint_is_valid(Waypoint *wpt);

/**
 * @return the active Flightplan, or NULL there is none
 */
Flightplan *flightplan_get_active();

/**
 * Lists all Flightplans in the filesystem.
 * @param list pointer to store the list of Flightplan names (as an array of char *)
 * @return the number of Flightplans in the list, or -1 if an error occurred
 */
i32 flightplan_list(char **list[]);

/**
 * Sets the active Flightplan.
 * @param flightplan the Flightplan to set
 */
void flightplan_set_active(Flightplan flightplan);

/**
 * Gets the JSON string of a Flightplan from the filesystem.
 * @param name the name of the Flightplan
 * @return the JSON string of the Flightplan, or NULL if it does not exist
 * @note The returned string must be freed by the caller.
 */
char *flightplan_get_json(const char *name);

/**
 * Saves a Flightplan to the filesystem as a JSON file.
 * @param name the name of the Flightplan
 * @param json the JSON string to save
 * @return whether the save was successful
 * @note This will overwrite any existing file with the same name.
 */
bool flightplan_save_json(const char *name, const char *json);

/**
 * Deletes a Flightplan from the filesystem.
 * @param name the name of the Flightplan
 * @return whether the delete was successful
 */
bool flightplan_delete(const char *name);

/**
 * Parses a Flightplan from a JSON string.
 * @param name the name of the Flightplan to parse
 * @param flightplan the Flightplan to populate
 * @param silent if true, suppresses log messages
 * @return the result of the parse attempt, if successful,
 */
FlightplanState flightplan_parse(const char *name, Flightplan *flightplan, bool silent);
