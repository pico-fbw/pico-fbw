/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "platform/flash.h"

#include "lib/parson.h"
#include "sys/log.h"
#include "sys/print.h"
#include "sys/version.h"

#include "flightplan.h"

#define FLIGHTPLAN_STORAGE_DIR "flightplans"
#define JSON_SCHEMA_V1                                                                                                 \
    "{\"version\":\"\",\"version_fw\":\"\",\"alt_samples\":0,\"waypoints\":"                                           \
    "[{\"lat\":0,\"lng\":0,\"alt\":0,\"speed\":0,\"drop\":0}]}"

static Flightplan active;
static bool isActive = false;

static inline bool state_is_error(FlightplanState state) {
    return state == FLIGHTPLAN_ERR_PARSE || state == FLIGHTPLAN_ERR_VERSION || state == FLIGHTPLAN_ERR_MEM;
}

static inline bool state_is_warning(FlightplanState state) {
    return state == FLIGHTPLAN_WARN_FW_VERSION;
}

bool waypoint_is_valid(Waypoint *wpt) {
    return fabs(wpt->lat) <= 90 && fabs(wpt->lng) <= 180 && wpt->alt >= 0 && wpt->alt <= 400 && wpt->speed >= 0 &&
           wpt->speed <= 100 && wpt->drop >= 0 && wpt->drop <= 60;
}

Flightplan *flightplan_get_active() {
    return isActive ? &active : NULL;
}

void flightplan_set_active(Flightplan flightplan) {
    if (isActive) {
        // Free the old active flightplan
        free(active.version);
        free(active.waypoints);
        free(active.name);
        free(active.json);
    }
    active = flightplan;
    isActive = true;
    log_message(TYPE_INFO, "Flightplan recieved!", -1, 0, false);
}

i32 flightplan_list(char **list[]) {
    lfs_dir_t dir;
    struct lfs_info info;
    if (lfs_dir_open(&lfs, &dir, FLIGHTPLAN_STORAGE_DIR) < 0) {
        return false;
    }
    u32 count = 0;
    *list = NULL;
    while (lfs_dir_read(&lfs, &dir, &info) > 0) {
        if (info.type != LFS_TYPE_REG) {
            continue;
        }
        char *name = strdup(info.name);
        char **temp = realloc(*list, (count + 1) * sizeof(char *));
        if (!name || !temp) {
            lfs_dir_close(&lfs, &dir);
            return -1;
        }
        // Remove the .json extension
        name[strlen(info.name) - 5] = '\0';
        *list = temp;
        // Store the name in the list
        (*list)[count] = name;
        count++;
    }
    lfs_dir_close(&lfs, &dir);
    return count;
}

char *flightplan_get_json(const char *name) {
    char path[LFS_NAME_MAX + 1];
    snprintf(path, sizeof(path), FLIGHTPLAN_STORAGE_DIR "/%s.json", name);
    // Get file info to determine the size of our buffer and open the file
    struct lfs_info info;
    if (lfs_stat(&lfs, path, &info) != LFS_ERR_OK) {
        return NULL;
    }
    lfs_file_t file;
    if (lfs_file_open(&lfs, &file, path, LFS_O_RDONLY) != LFS_ERR_OK) {
        return NULL;
    }
    // Read the file into a buffer
    char *json = malloc(info.size + 1);
    if (!json) {
        lfs_file_close(&lfs, &file);
        return NULL;
    }
    if (lfs_file_read(&lfs, &file, json, info.size) != (lfs_ssize_t)info.size) {
        lfs_file_close(&lfs, &file);
        free(json);
        return NULL;
    }
    lfs_file_close(&lfs, &file);
    json[info.size] = '\0'; // Terminate the string
    return json;
}

bool flightplan_save_json(const char *name, const char *json) {
    // Ensure storage directory exists
    i32 dir = lfs_mkdir(&lfs, FLIGHTPLAN_STORAGE_DIR);
    if (dir != LFS_ERR_OK && dir != LFS_ERR_EXIST) {
        return false;
    }
    // Write the JSON to a file in the storage directory, truncating any existing file if necessary
    char path[LFS_NAME_MAX + 1];
    snprintf(path, sizeof(path), FLIGHTPLAN_STORAGE_DIR "/%s.json", name);
    lfs_file_t file;
    if (lfs_file_open(&lfs, &file, path, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) != LFS_ERR_OK) {
        return false;
    }
    if (lfs_file_write(&lfs, &file, json, strlen(json)) != (lfs_ssize_t)strlen(json)) {
        lfs_file_close(&lfs, &file);
        return false;
    }
    lfs_file_close(&lfs, &file);
    return true;
}

bool flightplan_delete(const char *name) {
    char path[LFS_NAME_MAX + 1];
    snprintf(path, sizeof(path), FLIGHTPLAN_STORAGE_DIR "/%s.json", name);
    return lfs_remove(&lfs, path) == LFS_ERR_OK;
}

FlightplanState flightplan_parse(const char *name, Flightplan *flightplan, bool silent) {
    // Load the JSON file
    char *json = flightplan_get_json(name);
    if (!json) {
        if (!silent) {
            printpre("flightplan", "ERROR: failed to load flightplan");
        }
        return FLIGHTPLAN_ERR_LOAD;
    }
    FlightplanState state;
    // Ensure the recieved JSON matches the template schema for a valid flightplan
    JSON_Value *schema = json_parse_string(JSON_SCHEMA_V1);
    JSON_Value *root = json_parse_string(json);
    if (json_validate(schema, root) != JSONSuccess) {
        if (!silent) {
            printpre("flightplan", "ERROR: schema validation failed");
        }
        state = FLIGHTPLAN_ERR_PARSE;
        goto cleanup;
    }

    // Version
    JSON_Object *obj = json_value_get_object(root);
    const char *version = json_object_get_string(obj, "version");
    if (strcmp(version, FLIGHTPLAN_VERSION) != 0) {
        if (!silent) {
            printpre("flightplan", "ERROR: version mismatch");
        }
        state = FLIGHTPLAN_ERR_VERSION;
        goto cleanup;
    }
    flightplan->version = strdup(version);
    if (!flightplan->version) {
        goto oom;
    }

    // Firmware version
    const char *version_fw = json_object_get_string(obj, "version_fw");
    VersionCheck versionCheck = version_check((char *)version_fw);
    switch (versionCheck) {
        case VERSION_SAME:
        case VERSION_OLDER:
            break;
        case VERSION_NEWER:
            if (!silent) {
                printpre("flightplan", "a new firmware version is available");
            }
            state = FLIGHTPLAN_WARN_FW_VERSION;
            // Don't return here as this is just a warning, we should continue parsing
            break;
        default:
            if (!silent) {
                printpre("flightplan", "ERROR: firmware version check failed");
            }
            state = FLIGHTPLAN_ERR_PARSE;
            goto cleanup;
    }
    flightplan->version_fw = strdup(version_fw);
    if (!flightplan->version_fw) {
        goto oom;
    }

    // Altitude samples
    flightplan->alt_samples = json_object_get_number(obj, "alt_samples");
    if (flightplan->alt_samples < 0 || flightplan->alt_samples > 100) {
        if (!silent) {
            printpre("flightplan", "ERROR: invalid altitude samples");
        }
        state = FLIGHTPLAN_ERR_PARSE;
        goto cleanup;
    }
    // Only replace the state if there have been no warnings/errors up to this point
    if (flightplan->alt_samples != 0 && !state_is_warning(state) && !state_is_error(state)) {
        state = FLIGHTPLAN_STATUS_GPS_OFFSET;
    }

    // Waypoint array
    JSON_Array *waypoints = json_object_get_array(obj, "waypoints");
    flightplan->waypoint_count = json_array_get_count(waypoints);
    if (!silent) {
        printpre("flightplan", "flightplan contains %lu Waypoints\n", flightplan->waypoint_count);
    }
    flightplan->waypoints = calloc(flightplan->waypoint_count, sizeof(Waypoint));
    if (!flightplan->waypoints) {
        goto oom;
    }
    for (u32 i = 0; i < flightplan->waypoint_count; i++) {
        JSON_Object *waypoint = json_array_get_object(waypoints, i);
        flightplan->waypoints[i].lat = json_object_get_number(waypoint, "lat");
        flightplan->waypoints[i].lng = json_object_get_number(waypoint, "lng");
        flightplan->waypoints[i].alt = json_object_get_number(waypoint, "alt");
        flightplan->waypoints[i].speed = json_object_get_number(waypoint, "speed");
        flightplan->waypoints[i].drop = json_object_get_number(waypoint, "drop");
        if (!waypoint_is_valid(&flightplan->waypoints[i])) {
            if (!silent) {
                printpre("flightplan", "ERROR: Waypoint %lu contains invalid data", i + 1);
            }
            state = FLIGHTPLAN_ERR_PARSE;
            goto cleanup;
        }
    }

    // Copy metadata to be accessible later
    flightplan->name = strdup(name);
    flightplan->json = strdup(json);
    if (!flightplan->name || !flightplan->json) {
        goto oom;
    }

    // Make sure we don't overwrite the state if it's already set to a special value
    if (state != FLIGHTPLAN_STATUS_GPS_OFFSET && state != FLIGHTPLAN_WARN_FW_VERSION) {
        state = FLIGHTPLAN_STATUS_OK;
    }

cleanup:
    json_value_free(root);
    json_value_free(schema);
    free(json);
    return state;
oom:
    if (!silent) {
        printpre("flightplan", "ERROR: out of memory");
    }
    state = FLIGHTPLAN_ERR_MEM;
    goto cleanup;
}
