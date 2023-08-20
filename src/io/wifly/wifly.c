/**
 * Huge thank-you to Rasperry Pi (as a part of the Pico examples library) for providing much of the code used in Wi-Fly!
 * 
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "pico/config.h"
#ifdef RASPBERRYPI_PICO_W
    #include "pico/cyw43_arch.h"
#endif
#define JSMN_HEADER // Only define once!
#include "../../lib/jsmn.h"

#ifdef RASPBERRYPI_PICO_W
    #include "pcl/dhcp.h"
    #include "pcl/dns.h"
    #include "pcl/tcp.h"

    dhcp_server_t dhcp_server;
    dns_server_t dns_server;
    TCP_SERVER_T *state;
#endif

#include "../../config.h"
#include "../../version.h"

#include "wifly.h"

static WiflyStatus fplanStatus = WIFLY_STATUS_AWAITING;

static Waypoint *waypoints = NULL;
static uint waypoint_count = 0;

static char *fplanJson = NULL; // Once the flightplan is downloaded it will be stored here and NOT freed!

uint numGpsSamples = 0; // Holds the number of GPS samples to be taken (if other than zero) once the flightplan has been parsed

// TODO: somehow activate gps calibration function
// I tried to accomplish this for a few hours and I'm just giving up--all everything does is crash the Pico :(

static inline void url_decode(char *str) {
    char *p = str;
    char *q = str;
    while (*p != '\0') {
        if (*p == '%') {
            char hex[3];
            hex[0] = *(p + 1);
            hex[1] = *(p + 2);
            hex[2] = '\0';
            *q = strtol(hex, NULL, 16);
            p += 2;
        } else if (*p == '+') {
            *q = ' ';
        } else {
            *q = *p;
        }
        p++;
        q++;
    }
    *q = '\0';
}

#ifdef RASPBERRYPI_PICO_W

    void wifly_init() {
        state = calloc(1, sizeof(TCP_SERVER_T));
        if (!state) {
            FBW_DEBUG_printf("[wifly] ERROR: tcp failed to allocate state\n");
        }
        const char *ap_name = WIFLY_NETWORK_NAME;
        #ifdef WIFLY_NETWORK_USE_PASSWORD
            const char *password = WIFLY_NETWORK_PASSWORD;
        #else
            const char *password = NULL;
        #endif
        cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK);

        ip4_addr_t mask;
        IP4_ADDR(ip_2_ip4(&state->gw), 192, 168, 4, 1);
        IP4_ADDR(ip_2_ip4(&mask), 255, 255, 255, 0);
        dhcp_server_init(&dhcp_server, &state->gw, &mask);
        dns_server_init(&dns_server, &state->gw);
        if (!tcp_server_open(state)) {
            FBW_DEBUG_printf("[wifly] ERROR: tcp failed to open server\n");
        }
    }

    void wifly_deinit() {
        dns_server_deinit(&dns_server);
        dhcp_server_deinit(&dhcp_server);
        tcp_server_close(state);
        // cyw43_arch_deinit();
    }

#endif // RASPBERRYPI_PICO_W

int wifly_genPageContent(char *result, size_t max_result_len) {
    char color[8];
    char msg[130]; // change this if the longest message gets any larger, this is too small for me to implement malloc and such
    switch(fplanStatus) {
        case WIFLY_STATUS_AWAITING:
            snprintf(color, sizeof(color), WIFLY_HEX_INACTIVE);
            snprintf(msg, sizeof(msg), "Awaiting flightplan...");
            break;
        case WIFLY_STATUS_OK:
            snprintf(color, sizeof(color), WIFLY_HEX_OK);
            snprintf(msg, sizeof(msg), "Flightplan successfully uploaded!");
            break;
        case WIFLY_STATUS_GPS_OFFSET:
            snprintf(color, sizeof(color), WIFLY_HEX_OK);
            snprintf(msg, sizeof(msg), "Flightplan successfully uploaded, please wait for the altitude offset to calculate.<br>Estimated time: %ds", numGpsSamples);
            break;
        case WIFLY_ERR_PARSE:
            snprintf(color, sizeof(color), WIFLY_HEX_ERR);
            snprintf(msg, sizeof(msg), "<b>Error:</b> parse. Check formatting and try again.");
            break;
        case WIFLY_ERR_VERSION:
            snprintf(color, sizeof(color), WIFLY_HEX_ERR);
            snprintf(msg, sizeof(msg), "<b>Error:</b> flightplan version incompatable! Please update your firmware.");
            break;
        case WIFLY_ERR_MEM:
            snprintf(color, sizeof(color), WIFLY_HEX_ERR);
            snprintf(msg, sizeof(msg), "<b>Error:</b> out of memory! Please restart and try again.");
            break;
        case WIFLY_WARN_FW_VERSION:
            snprintf(color, sizeof(color), WIFLY_HEX_WARN);
            snprintf(msg, sizeof(msg), "<b>Warning: </b> there is a new firmware version available!");
            break;   
        default:
            return 0;
    }
    return snprintf(result, max_result_len, PAGE_CONTENT, color, msg);
}

bool wifly_parseFplan(const char *fplan) {
    // Check if we have already parsed the flightplan
    if (fplanJson == NULL) {
        // Find the start of the JSON string
        const char *json_start = strstr(fplan, FPLAN_PARAM);
        if (!json_start) {
            FBW_DEBUG_printf("[wifly] ERROR: prefix '%s' not found!\n", FPLAN_PARAM);
            fplanStatus = WIFLY_ERR_PARSE;
            return false;
        }
        // Move the pointer to the start of the JSON string
        // This effectively skips the "fplan=" prefix so we don't confuse the JSON parser later
        json_start += strlen(FPLAN_PARAM);

        // Now, URL decode the input string so we can parse it in JSON format instead of URL
        char decoded[strlen(json_start) + 1];
        strcpy(decoded, json_start);
        url_decode(decoded);
        #if WIFLY_DUMP_DATA
            printf("\n[wifly] Flightplan data URL encoded: %s\n", json_start);
            printf("\n[wifly] Flightplan data URL decoded: %s\n\n", decoded);
        #endif

        // Initialize JSON parsing logic
        jsmn_parser parser;
        jsmntok_t tokens[strlen(decoded)];
        jsmn_init(&parser);
        int token_count = jsmn_parse(&parser, decoded, strlen(decoded), tokens, sizeof(tokens)/sizeof(tokens[0]));
        if (token_count < 0) {
            FBW_DEBUG_printf("[wifly] ERROR: no valid tokens found!\n");
            fplanStatus = WIFLY_ERR_PARSE;
            return false;
        }
        if (strlen(decoded) == 0 || decoded[0] != '{' || decoded[strlen(decoded) - 1] != '}') {
            FBW_DEBUG_printf("[wifly] ERROR: JSON start/end not found!\n");
            fplanStatus = WIFLY_ERR_PARSE;
            return false;
        }

        WiflyStatus status = WIFLY_STATUS_OK;
        bool has_version, has_version_fw, has_gps_samples = false;
        // Process all tokens and extract any needed data; things here are mostly self-explanatory
        for (uint i = 0; i < token_count; i++) {
            // Token field (name) iteration
            if (tokens[i].type == JSMN_STRING) {
                // Find version and check it
                char field_name[25];
                strncpy(field_name, decoded + tokens[i].start, tokens[i].end - tokens[i].start);
                field_name[tokens[i].end - tokens[i].start] = '\0';
                if (strcmp(field_name, "version") == 0) {
                    char version[25];
                    strncpy(version, decoded + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
                    version[tokens[i + 1].end - tokens[i + 1].start] = '\0';
                    WIFLY_DEBUG_printf("[wifly] Flightplan version: %s\n", version);
                    if (strcmp(version, WIFLY_VERSION) != 0) {
                        FBW_DEBUG_printf("[wifly] ERROR: flightplan version incompatable!\n");
                        fplanStatus = WIFLY_ERR_VERSION;
                        return false;
                    }
                    has_version = true;
                } else if (strcmp(field_name, "gps_samples") == 0) {
                    char gpsSamples[25];
                    strncpy(gpsSamples, decoded + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
                    gpsSamples[tokens[i + 1].end - tokens[i + 1].start] = '\0';
                    numGpsSamples = atoi(gpsSamples);
                    // We expect a value between zero and 100 (only calculate if non-zero)
                    if (numGpsSamples <= 100 && numGpsSamples >= 0) {
                        WIFLY_DEBUG_printf("[wifly] GPS num offset samples: %s\n", gpsSamples);
                        if (numGpsSamples != 0) {
                            status = WIFLY_STATUS_GPS_OFFSET;
                        }
                    } else {
                        status = WIFLY_ERR_PARSE;
                        return false;
                    }
                    has_gps_samples = true;
                } else if (strcmp(field_name, "version_fw") == 0) {
                    // Even though version_fw comes before gps_samples in the JSON format, the parse is ordered in this way to display the warning message
                    char versionFw[25];
                    strncpy(versionFw, decoded + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
                    versionFw[tokens[i + 1].end - tokens[i + 1].start] = '\0';
                    WIFLY_DEBUG_printf("[wifly] Firmware version: %s\n", versionFw);
                    // Check if the version is up-to-date
                    if (strncmp(versionFw, PICO_FBW_VERSION, 5) == 0) {
                        // Version numbering is up-to-date but is it a prerelease?
                        const char *versionSuffix = strstr(PICO_FBW_VERSION, "-");
                        if (versionSuffix != NULL) {
                            versionSuffix += strlen("-");
                            if (strncmp(versionSuffix, "alpha", strlen("alpha")) == 0) {
                                FBW_DEBUG_printf("[wifly] There is a non-alpha release of %s available!\n", PICO_FBW_VERSION);
                                status = WIFLY_WARN_FW_VERSION;
                            } else if (strncmp(versionSuffix, "beta", strlen("beta")) == 0) {
                                FBW_DEBUG_printf("[wifly] There is a non-beta release of %s available!\n", PICO_FBW_VERSION);
                                status = WIFLY_WARN_FW_VERSION;
                            }
                        } else {
                            WIFLY_DEBUG_printf("[wifly] Version is up-to-date\n");
                        }
                    } else if (strncmp(versionFw, PICO_FBW_VERSION, 5) > 0) {
                        FBW_DEBUG_printf("[wifly] WARNING: firmware out of date\n");
                        status = WIFLY_WARN_FW_VERSION;
                    } else if (strncmp(versionFw, PICO_FBW_VERSION, 5) < 0) {
                        FBW_DEBUG_printf("[wifly] Version is a prerelease\nThanks for contributing :)\n");
                    }
                    has_version_fw = true;
                } else if (strcmp(field_name, "waypoints") == 0) {
                    if (tokens[i + 1].type == JSMN_ARRAY) {
                        waypoint_count = tokens[i + 1].size;
                        WIFLY_DEBUG_printf("[wifly] Flightplan contains %d waypoints\n", waypoint_count);
                        // Allocate memory for the waypoints array
                        waypoints = calloc(waypoint_count, sizeof(Waypoint));
                        if (waypoints == NULL) {
                            FBW_DEBUG_printf("[wifly] ERROR: memory allocation for waypoint data failed!\n");
                            fplanStatus = WIFLY_ERR_MEM;
                            return false;
                        }
                        uint waypoint_token_index = i + 2; // Skip the array token
                        // Waypoint iteration
                        for (uint w = 0; w < waypoint_count; w++) {
                            #if WIFLY_DUMP_DATA
                                printf("Processing waypoint %d:\n", w + 1);
                            #endif
                            if (tokens[waypoint_token_index].type == JSMN_OBJECT) {
                                uint waypoint_token_count = tokens[waypoint_token_index].size;
                                uint waypoint_field_token_index = waypoint_token_index + 1; // Skip the object token
                                Waypoint waypoint;
                                // Waypoint field iteration
                                for (uint j = 0; j < waypoint_token_count; j++) {
                                    if (tokens[waypoint_field_token_index].type == JSMN_STRING) {
                                        char waypoint_field_name[4];
                                        strncpy(waypoint_field_name, decoded + tokens[waypoint_field_token_index].start,
                                                tokens[waypoint_field_token_index].end - tokens[waypoint_field_token_index].start);
                                        waypoint_field_name[tokens[waypoint_field_token_index].end - tokens[waypoint_field_token_index].start] = '\0';
                                        if (strcmp(waypoint_field_name, "lat") == 0) {
                                            char lat[17];
                                            strncpy(lat, decoded + tokens[waypoint_field_token_index + 1].start,
                                                    tokens[waypoint_field_token_index + 1].end - tokens[waypoint_field_token_index + 1].start);
                                            lat[tokens[waypoint_field_token_index + 1].end - tokens[waypoint_field_token_index + 1].start] = '\0';
                                            // Store the latitude value into a waypoint struct
                                            waypoint.lat = atof(lat);
                                            #if WIFLY_DUMP_DATA
                                                printf("Latitude: %s\n", lat);
                                            #endif
                                        } else if (strcmp(waypoint_field_name, "lng") == 0) {
                                            char lng[17];
                                            strncpy(lng, decoded + tokens[waypoint_field_token_index + 1].start,
                                                    tokens[waypoint_field_token_index + 1].end - tokens[waypoint_field_token_index + 1].start);
                                            lng[tokens[waypoint_field_token_index + 1].end - tokens[waypoint_field_token_index + 1].start] = '\0';
                                            waypoint.lng = atof(lng);
                                            #if WIFLY_DUMP_DATA
                                                printf("Longitude: %s\n", lng);
                                            #endif    
                                        } else if (strcmp(waypoint_field_name, "alt") == 0) {
                                            char alt[4];
                                            strncpy(alt, decoded + tokens[waypoint_field_token_index + 1].start,
                                                    tokens[waypoint_field_token_index + 1].end - tokens[waypoint_field_token_index + 1].start);
                                            alt[tokens[waypoint_field_token_index + 1].end - tokens[waypoint_field_token_index + 1].start] = '\0';
                                            waypoint.alt = atoi(alt);
                                            #if WIFLY_DUMP_DATA
                                                printf("Altitude: %s\n", alt);
                                            #endif    
                                        } else {
                                            FBW_DEBUG_printf("[wifly] ERROR: waypoint field name not recognized!\n");
                                            fplanStatus = WIFLY_ERR_PARSE;
                                            return false;
                                        }
                                        // Advance to the next field
                                        waypoint_field_token_index += 2;
                                    }
                                }
                                // Check if the waypoint data is valid (we got everything we expected)
                                if (waypoint.lat == 0 || waypoint.lng == 0 || waypoint.alt == 0) {
                                    FBW_DEBUG_printf("[wifly] ERROR: waypoint data is invalid!\n");
                                    fplanStatus = WIFLY_ERR_PARSE;
                                    return false;
                                }
                                // Store the generated waypoint into the master array
                                waypoints[w] = waypoint;
                                // Advance to the next waypoint
                                waypoint_token_index += 7;
                            } else {
                                FBW_DEBUG_printf("[wifly] ERROR: waypoint token type not recognized!\n");
                                fplanStatus = WIFLY_ERR_PARSE;
                                return false;
                            }
                        }
                    } else {
                        FBW_DEBUG_printf("[wifly] ERROR: waypoints array not found!\n");
                        fplanStatus = WIFLY_ERR_PARSE;
                        return false;
                    }
                }
            }
        }
        if (!has_version || !has_version_fw || !has_gps_samples) {
            WIFLY_DEBUG_printf("[wifly] ERROR: crucial fields not present in flightplan!\n");
            fplanStatus = WIFLY_ERR_PARSE;
            return false;
        }
        WIFLY_DEBUG_printf("[wifly] Waypoint data:\n");
        for (int i = 0; i < waypoint_count; i++) {
            WIFLY_DEBUG_printf("Waypoint %d: lat=%.10f, lng=%.10f, alt=%d\n", i + 1, waypoints[i].lat, waypoints[i].lng, waypoints[i].alt);
        }
        // Save the flightplan JSON string
        fplanJson = malloc(strlen(decoded) + 1);
        if (fplanJson == NULL) {
            FBW_DEBUG_printf("[wifly] ERROR: memory allocation for flightplan JSON failed!\n");
            fplanStatus = WIFLY_ERR_MEM;
            return false;
        }
        strcpy(fplanJson, decoded);
        fplanStatus = status;
        if (fplanStatus == WIFLY_STATUS_OK || fplanStatus == WIFLY_STATUS_GPS_OFFSET || fplanStatus == WIFLY_WARN_FW_VERSION) {
            return true;
        } else {
            return false;
        }
    } else {
        return true;
    }
}

Waypoint *wifly_getFplan() { return waypoints; }

const char *wifly_getFplanJson() { return fplanJson; }

uint wifly_getWaypointCount() { return waypoint_count; }
