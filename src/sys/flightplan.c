/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "lib/parson.h"

#include "sys/log.h"
#include "sys/print.h"
#include "sys/version.h"

#include "flightplan.h"

// Flightplan v1.0 schema
#define JSON_SCHEMA                                                                                                            \
    "{\"version\":\"\",\"version_fw\":\"\",\"alt_samples\":0,\"waypoints\":"                                                   \
    "[{\"lat\":0,\"lng\":0,\"alt\":0,\"speed\":0,\"drop\":0}]}"

static Flightplan flightplan;
static FlightplanError state = FLIGHTPLAN_STATUS_AWAITING; // Current state of the flightplan parsage

bool waypoint_is_valid(Waypoint *wpt) {
    return fabs(wpt->lat) <= 90 && fabs(wpt->lng) <= 180 && wpt->alt >= -5 && wpt->alt <= 400 && wpt->speed >= -5 &&
           wpt->speed <= 100 && wpt->drop >= 0 && wpt->drop <= 60;
}

bool flightplan_was_parsed() {
    return (state == FLIGHTPLAN_STATUS_OK || state == FLIGHTPLAN_STATUS_GPS_OFFSET || state == FLIGHTPLAN_WARN_FW_VERSION);
}

FlightplanError flightplan_parse(const char *json, bool silent) {
    if (state != FLIGHTPLAN_STATUS_AWAITING)
        return FLIGHTPLAN_ERR_ALREADY_PARSED;
    // Ensure the recieved JSON matches the template schema for a valid flightplan
    JSON_Value *schema = json_parse_string(JSON_SCHEMA);
    JSON_Value *root = json_parse_string(json);
    if (json_validate(schema, root) != JSONSuccess) {
        if (!silent)
            printpre("flightplan", "ERROR: schema validation failed");
        state = FLIGHTPLAN_ERR_PARSE;
        goto cleanup;
    }
    // Version
    JSON_Object *obj = json_value_get_object(root);
    const char *version = json_object_get_string(obj, "version");
    if (strcmp(version, FLIGHTPLAN_VERSION) != 0) {
        if (!silent)
            printpre("flightplan", "ERROR: version mismatch");
        state = FLIGHTPLAN_ERR_VERSION;
        goto cleanup;
    }
    flightplan.version = malloc(strlen(version) + 1);
    if (!flightplan.version) {
        if (!silent)
            printpre("flightplan", "ERROR: out of memory");
        state = FLIGHTPLAN_ERR_MEM;
        goto cleanup;
    }
    strcpy(flightplan.version, version);
    // Firmware version
    const char *version_fw = json_object_get_string(obj, "version_fw");
    VersionCheck versionCheck = version_check((char *)version_fw);
    switch (versionCheck) {
        case VERSION_SAME:
        case VERSION_OLDER:
            break;
        case VERSION_NEWER:
            if (!silent)
                printpre("flightplan", "a new firmware version is available");
            state = FLIGHTPLAN_WARN_FW_VERSION;
            // Don't return here as this is just a warning, we should continue parsing
            break;
        default:
            if (!silent)
                printpre("flightplan", "ERROR: firmware version check failed");
            state = FLIGHTPLAN_ERR_PARSE;
            goto cleanup;
    }
    // Altitude samples
    flightplan.alt_samples = json_object_get_number(obj, "alt_samples");
    if (flightplan.alt_samples < 0 || flightplan.alt_samples > 100) {
        if (!silent)
            printpre("flightplan", "ERROR: invalid altitude samples");
        state = FLIGHTPLAN_ERR_PARSE;
        goto cleanup;
    }
    // Waypoint array
    JSON_Array *waypoints = json_object_get_array(obj, "waypoints");
    flightplan.waypoint_count = json_array_get_count(waypoints);
    if (!silent)
        printpre("flightplan", "flightplan contains %lu Waypoints\n", flightplan.waypoint_count);
    flightplan.waypoints = calloc(flightplan.waypoint_count, sizeof(Waypoint));
    if (!flightplan.waypoints) {
        if (!silent)
            printpre("flightplan", "ERROR: out of memory");
        state = FLIGHTPLAN_ERR_MEM;
        goto cleanup;
    }
    for (u32 i = 0; i < flightplan.waypoint_count; i++) {
        JSON_Object *waypoint = json_array_get_object(waypoints, i);
        flightplan.waypoints[i].lat = json_object_get_number(waypoint, "lat");
        flightplan.waypoints[i].lng = json_object_get_number(waypoint, "lng");
        flightplan.waypoints[i].alt = json_object_get_number(waypoint, "alt");
        flightplan.waypoints[i].speed = json_object_get_number(waypoint, "speed");
        flightplan.waypoints[i].drop = json_object_get_number(waypoint, "drop");
        if (!waypoint_is_valid(&flightplan.waypoints[i])) {
            if (!silent)
                printpre("flightplan", "ERROR: Waypoint %lu contains invalid data", i + 1);
            state = FLIGHTPLAN_ERR_PARSE;
            goto cleanup;
        }
    }

    // Copy JSON string to be accessible later
    flightplan.json = malloc(strlen(json) + 1);
    if (!flightplan.json) {
        if (!silent)
            printpre("flightplan", "ERROR: out of memory");
        state = FLIGHTPLAN_ERR_MEM;
        return state;
    }
    strcpy(flightplan.json, json);

    if (state != FLIGHTPLAN_STATUS_GPS_OFFSET && state != FLIGHTPLAN_WARN_FW_VERSION)
        // Make sure we don't overwrite the state if it's already set
        state = FLIGHTPLAN_STATUS_OK;
    log_message(TYPE_INFO, "Flightplan recieved!", -1, 0, false);

cleanup:
    json_value_free(root);
    json_value_free(schema);
    return state;
}

Flightplan *flightplan_get() {
    if (flightplan_was_parsed())
        return &flightplan;
    return NULL;
}

FlightplanError flightplan_state() {
    return state;
}
