/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <stdlib.h>
#include <string.h>

#include "lib/jsmn.h"
#define JSMN_HEADER // Only define once!

#include "sys/configuration.h"
#include "sys/log.h"
#include "sys/print.h"
#include "sys/version.h"

#include "flightplan.h"

static Flightplan flightplan;
static FlightplanError state = FPLAN_STATUS_AWAITING; // Current state of the flightplan (awaiting? pasred, OK? parsed, error?)

FlightplanError flightplan_parse(const char *json) {
    // Initialize JSON parsing logic
    jsmn_parser parser;
    jsmntok_t tokens[strlen(json)];
    jsmn_init(&parser);
    i32 tokenCount = jsmn_parse(&parser, json, strlen(json), tokens, count_of(tokens));
    if (strlen(json) == 0 || json[0] != '{' || json[strlen(json) - 1] != '}') {
        print("[fplan] ERROR: JSON start/end not found!");
        state = FPLAN_ERR_PARSE;
        return state;
    }
    if (tokenCount < 0) {
        print("[fplan] ERROR: no valid tokens found!");
        state = FPLAN_ERR_PARSE;
        return state;
    }

    FlightplanError status = FPLAN_STATUS_OK;
    bool has_version = false, has_version_fw = false, has_alt_samples = false;
    // Process all tokens and extract any needed data; things here are mostly self-explanatory
    for (u32 i = 0; i < tokenCount; i++) {
        // Token field (name) iteration
        if (tokens[i].type == JSMN_STRING) {
            char field_name[25];
            strncpy(field_name, json + tokens[i].start, tokens[i].end - tokens[i].start);
            field_name[tokens[i].end - tokens[i].start] = '\0';
            if (strcmp(field_name, "version") == 0) {
                char version[25];
                strncpy(version, json + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
                version[tokens[i + 1].end - tokens[i + 1].start] = '\0';

                print("[fplan] Flightplan version: %s", version);
                if (strcmp(version, FPLAN_VERSION) != 0) {
                    print("[fplan] ERROR: flightplan version incompatable!");
                    state = FPLAN_ERR_VERSION;
                    return state;
                }
                flightplan.version = malloc(strlen(version) + 1);
                if (!flightplan.version) {
                    print("[fplan] ERROR: memory allocation for version data failed!");
                    state = FPLAN_ERR_MEM;
                    return state;
                }
                strcpy(flightplan.version, version);
                has_version = true;
            } else if (strcmp(field_name, "version_fw") == 0) {
                char versionFw[25];
                strncpy(versionFw, json + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
                versionFw[tokens[i + 1].end - tokens[i + 1].start] = '\0';

                print("[fplan] Firmware version: %s", versionFw);
                i32 versionCheck = version_check(versionFw);
                if (versionCheck < 0) {
                    if (versionCheck < -1) {
                        print("[fplan] ERROR: version check failed!");
                        status = FPLAN_ERR_PARSE;
                        return status;
                    } else {
                        print("[fplan] WARNING: a new pico-fbw firmware version is available, please download it!");
                        status = FPLAN_WARN_FW_VERSION;
                    }
                }
                flightplan.version_fw = malloc(strlen(versionFw) + 1);
                if (!flightplan.version_fw) {
                    print("[fplan] ERROR: memory allocation for firmware version data failed!");
                    state = FPLAN_ERR_MEM;
                    return state;
                }
                strcpy(flightplan.version_fw, versionFw);
                has_version_fw = true;
            } else if (strcmp(field_name, "alt_samples") == 0) {
                char altSamples[25];
                strncpy(altSamples, json + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
                altSamples[tokens[i + 1].end - tokens[i + 1].start] = '\0';
                flightplan.alt_samples = atoi(altSamples);

                // We expect a value between zero and 100 (only calculate if non-zero)
                if (flightplan.alt_samples <= 100 && flightplan.alt_samples >= 0) {
                    print("[fplan] Num alt samples: %d", flightplan.alt_samples);
                    if (flightplan.alt_samples != 0 && status == FPLAN_STATUS_OK)
                        status = FPLAN_STATUS_GPS_OFFSET; // Only replace the status if it is still OK (no warnings yet)
                } else {
                    status = FPLAN_ERR_PARSE;
                    return state;
                }
                has_alt_samples = true;
            } else if (strcmp(field_name, "waypoints") == 0) {
                if (tokens[i + 1].type == JSMN_ARRAY) {
                    flightplan.waypoint_count = tokens[i + 1].size;
                    print("[fplan] Flightplan contains %d waypoints", flightplan.waypoint_count);
                    // Allocate memory for the waypoints array
                    flightplan.waypoints = calloc(flightplan.waypoint_count, sizeof(Waypoint));
                    if (!flightplan.waypoints) {
                        print("[fplan] ERROR: memory allocation for Waypoint data failed!");
                        state = FPLAN_ERR_MEM;
                        return state;
                    }
                    u32 waypoint_token_index = i + 2; // Skip the array token
                    // Waypoint iteration
                    for (u32 w = 0; w < flightplan.waypoint_count; w++) {
                        print("[fplan] Processing Waypoint %d:", w + 1);
                        if (tokens[waypoint_token_index].type == JSMN_OBJECT) {
                            u32 waypoint_tokenCount = tokens[waypoint_token_index].size;
                            u32 waypoint_field_token_index = waypoint_token_index + 1; // Skip the object token
                            Waypoint waypoint = {
                                .lat = INFINITY,
                                .lng = INFINITY,
                                .alt = INT16_MIN,
                                .speed = INFINITY,
                                .drop = 0,
                            };
                            // Waypoint field iteration
                            for (u32 j = 0; j < waypoint_tokenCount; j++) {
                                if (tokens[waypoint_field_token_index].type == JSMN_STRING) {
                                    char waypoint_field_name[5];
                                    strncpy(waypoint_field_name, json + tokens[waypoint_field_token_index].start,
                                            tokens[waypoint_field_token_index].end - tokens[waypoint_field_token_index].start);
                                    waypoint_field_name[tokens[waypoint_field_token_index].end -
                                                        tokens[waypoint_field_token_index].start] = '\0';

                                    if (strcmp(waypoint_field_name, "lat") == 0) {
                                        char lat[25];
                                        strncpy(lat, json + tokens[waypoint_field_token_index + 1].start,
                                                tokens[waypoint_field_token_index + 1].end -
                                                    tokens[waypoint_field_token_index + 1].start);
                                        lat[tokens[waypoint_field_token_index + 1].end -
                                            tokens[waypoint_field_token_index + 1].start] = '\0';

                                        // Store the latitude value into a Waypoint
                                        waypoint.lat = strtold(lat, NULL);
                                        print("Latitude: %s", lat);
                                    } else if (strcmp(waypoint_field_name, "lng") == 0) {
                                        char lng[25];
                                        strncpy(lng, json + tokens[waypoint_field_token_index + 1].start,
                                                tokens[waypoint_field_token_index + 1].end -
                                                    tokens[waypoint_field_token_index + 1].start);
                                        lng[tokens[waypoint_field_token_index + 1].end -
                                            tokens[waypoint_field_token_index + 1].start] = '\0';

                                        waypoint.lng = strtold(lng, NULL);
                                        print("Longitude: %s", lng);
                                    } else if (strcmp(waypoint_field_name, "alt") == 0) {
                                        char alt[4];
                                        strncpy(alt, json + tokens[waypoint_field_token_index + 1].start,
                                                tokens[waypoint_field_token_index + 1].end -
                                                    tokens[waypoint_field_token_index + 1].start);
                                        alt[tokens[waypoint_field_token_index + 1].end -
                                            tokens[waypoint_field_token_index + 1].start] = '\0';

                                        waypoint.alt = atoi(alt);
                                        print("Altitude: %s", alt);
                                    } else if (strcmp(waypoint_field_name, "spd") == 0) {
                                        char spd[25];
                                        strncpy(spd, json + tokens[waypoint_field_token_index + 1].start,
                                                tokens[waypoint_field_token_index + 1].end -
                                                    tokens[waypoint_field_token_index + 1].start);
                                        spd[tokens[waypoint_field_token_index + 1].end -
                                            tokens[waypoint_field_token_index + 1].start] = '\0';

                                        waypoint.speed = strtof(spd, NULL);
                                        print("Speed: %s", spd);
                                    } else if (strcmp(waypoint_field_name, "drp") == 0) {
                                        char drp[4];
                                        strncpy(drp, json + tokens[waypoint_field_token_index + 1].start,
                                                tokens[waypoint_field_token_index + 1].end -
                                                    tokens[waypoint_field_token_index + 1].start);
                                        drp[tokens[waypoint_field_token_index + 1].end -
                                            tokens[waypoint_field_token_index + 1].start] = '\0';

                                        waypoint.drop = atoi(drp);
                                        print("Drop: %s", drp);
                                    } else {
                                        print("[fplan] ERROR: Waypoint field name not recognized!");
                                        state = FPLAN_ERR_PARSE;
                                        return false;
                                    }
                                    // Advance to the next field
                                    waypoint_field_token_index += 2;
                                }
                            }
                            // Check if the Waypoint data is valid (aka that we got everything we expect and need)
                            if (waypoint.lat == INFINITY || waypoint.lng == INFINITY || waypoint.alt == INT16_MIN ||
                                waypoint.speed == INFINITY) {
                                print("[fplan] ERROR: Waypoint data is invalid!");
                                state = FPLAN_ERR_PARSE;
                                return state;
                            }
                            // Store the parsed Waypoint into the master array
                            flightplan.waypoints[w] = waypoint;
                            // Advance to the next waypoint
                            waypoint_token_index += WAYPOINT_NUM_FIELDS * 2 + 1; // + 1 for object token yet again
                        } else {
                            print("[fplan] ERROR: Waypoint token type not recognized!");
                            state = FPLAN_ERR_PARSE;
                            return state;
                        }
                    }
                } else {
                    print("[fplan] ERROR: waypoints array not found!");
                    state = FPLAN_ERR_PARSE;
                    return state;
                }
            }
        }
    }
    if (!has_version || !has_version_fw || !has_alt_samples) {
        print("[fplan] ERROR: flightplan missing data!");
        state = FPLAN_ERR_PARSE;
        return state;
    }

    print("[fplan] Waypoint data:");
    for (i32 i = 0; i < flightplan.waypoint_count; i++) {
        print("Waypoint #%d: lat=%.10f, lng=%.10f, alt=%d, speed=%f, drop=%d", i + 1, flightplan.waypoints[i].lat,
              flightplan.waypoints[i].lng, flightplan.waypoints[i].alt, flightplan.waypoints[i].speed,
              flightplan.waypoints[i].drop);
    }
    // Save the flightplan JSON string
    flightplan.json = malloc(strlen(json) + 1);
    if (!flightplan.json) {
        print("[fplan] ERROR: memory allocation for flightplan JSON failed!");
        state = FPLAN_ERR_MEM;
        return false;
    }
    strcpy(flightplan.json, json);
    state = status;
    if (state == FPLAN_STATUS_OK || state == FPLAN_STATUS_GPS_OFFSET || state == FPLAN_WARN_FW_VERSION)
        log_message(INFO, "Flightplan recieved!", -1, 0, false);
    return state;
}

bool flightplan_was_parsed() {
    return state == FPLAN_STATUS_OK || state == FPLAN_STATUS_GPS_OFFSET || state == FPLAN_WARN_FW_VERSION;
}

Flightplan *flightplan_get() {
    if (flightplan_was_parsed())
        return &flightplan;
    return NULL;
}

FlightplanError flightplan_state() { return state; }
