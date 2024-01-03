// Huge thank-you to Raspberry Pi (as a part of the Pico examples library) for providing much of the code used in Wi-Fly!

/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 * 
 * This file utilizes code under the BSD-3-Clause License. See "LICENSE" for details.
*/

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "pico/config.h"
#include "pico/platform.h"
#include "pico/types.h"
#ifdef RASPBERRYPI_PICO_W
    #include "pico/cyw43_arch.h"
#endif

#ifdef RASPBERRYPI_PICO_W
    #include "tcp_ip/dhcp.h"
    #include "tcp_ip/dns.h"
    #include "tcp_ip/tcp.h"

    dhcp_server_t dhcp_server;
    dns_server_t dns_server;
    TCP_SERVER_T *state;
#endif

#include "../io/flash.h"
#include "../io/gps.h"

#include "../lib/jsmn.h"
#define JSMN_HEADER // Only define once!

#include "../sys/info.h"
#include "../sys/log.h"

#include "wifly.h"

static WiflyStatus fplanStatus = WIFLY_STATUS_AWAITING;

static Waypoint *waypoints = NULL;
static uint waypoint_count = 0;

static char *fplanJson = NULL; // Once the flightplan is downloaded it will be stored here and NOT freed!

int numAltSamples = -1; // Holds the number of GPS samples to be taken (if other than zero) once the flightplan has been parsed

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
            if (print.fbw) printf("[wifly] ERROR: tcp failed to allocate state\n");
        }
        const char *ap_name = flash.wifly_ssid;
        const char *password = NULL;
        if ((WiflyStatus)flash.general[GENERAL_WIFLY_STATUS] == WIFLY_ENABLED_PASS) {
            password = flash.wifly_pass;
        }
        cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK);

        ip4_addr_t mask;
        IP4_ADDR(ip_2_ip4(&state->gw), 192, 168, 4, 1);
        IP4_ADDR(ip_2_ip4(&mask), 255, 255, 255, 0);
        dhcp_server_init(&dhcp_server, &state->gw, &mask);
        dns_server_init(&dns_server, &state->gw);
        if (!tcp_server_open(state)) {
            if (print.fbw) printf("[wifly] ERROR: tcp failed to open server\n");
        }
    }

    void wifly_deinit() {
        dns_server_deinit(&dns_server);
        dhcp_server_deinit(&dhcp_server);
        tcp_server_close(state);
    }

    int wifly_genPageContent(char *result, size_t max_result_len) {
        char color[8];
        char msg[165]; // change this if the longest message gets any larger, this is too small for me to implement dynamic mem
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
                snprintf(msg, sizeof(msg), "Flightplan successfully uploaded!<br><br>"
                                           "When ready, please engage auto mode to calibrate the GPS."
                                           "<br>Auto mode will be automatically disengaged once complete.");
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
                snprintf(msg, sizeof(msg), "There is a new firmware version available!<br>"
                                           "Download it <a href=\"https://github.com/pico-fbw/pico-fbw/releases/latest\">here</a>.");
                break;   
            default:
                return 0;
        }
        return snprintf(result, max_result_len, PAGE_CONTENT, color, msg);
    }

#endif // RASPBERRYPI_PICO_W

bool wifly_parseFplan(const char *fplan) {
    if (!fplanJson) {
        // If we haven't already parsed, find the start of the JSON string
        const char *json_start = strstr(fplan, FPLAN_PARAM);
        if (!json_start) {
            if (print.fbw) printf("[wifly] ERROR: '%s' prefix not found!\n", FPLAN_PARAM);
            fplanStatus = WIFLY_ERR_PARSE;
            return false;
        }
        // Move the pointer to the start of the JSON string, this effectively skips the "fplan=" prefix
        json_start += strlen(FPLAN_PARAM);

        // URL decode the input string so we can parse it in JSON format instead of URL
        char decoded[strlen(json_start) + 1];
        strcpy(decoded, json_start);
        url_decode(decoded);
        if (print.wifly) printf("\n[wifly] Flightplan data URL encoded: %s\n", json_start);
        if (print.wifly) printf("\n[wifly] Flightplan data URL decoded: %s\n\n", decoded);

        // Initialize JSON parsing logic
        jsmn_parser parser;
        jsmntok_t tokens[strlen(decoded)];
        jsmn_init(&parser);
        int token_count = jsmn_parse(&parser, decoded, strlen(decoded), tokens, count_of(tokens));
        if (strlen(decoded) == 0 || decoded[0] != '{' || decoded[strlen(decoded) - 1] != '}') {
            if (print.fbw) printf("[wifly] ERROR: JSON start/end not found!\n");
            fplanStatus = WIFLY_ERR_PARSE;
            return false;
        }
        if (token_count < 0) {
            if (print.fbw) printf("[wifly] ERROR: no valid tokens found!\n");
            fplanStatus = WIFLY_ERR_PARSE;
            return false;
        }

        WiflyStatus status = WIFLY_STATUS_OK;
        bool has_version = false, has_version_fw = false, has_alt_samples = false;
        // Process all tokens and extract any needed data; things here are mostly self-explanatory
        for (uint i = 0; i < token_count; i++) {
            // Token field (name) iteration
            if (tokens[i].type == JSMN_STRING) {
                char field_name[25];
                strncpy(field_name, decoded + tokens[i].start, tokens[i].end - tokens[i].start);
                field_name[tokens[i].end - tokens[i].start] = '\0';
                if (strcmp(field_name, "version") == 0) {
                    char version[25];
                    strncpy(version, decoded + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
                    version[tokens[i + 1].end - tokens[i + 1].start] = '\0';

                    if (print.wifly) printf("[wifly] Flightplan version: %s\n", version);
                    if (strcmp(version, WIFLY_VERSION) != 0) {
                        if (print.fbw) printf("[wifly] ERROR: flightplan version incompatable!\n");
                        fplanStatus = WIFLY_ERR_VERSION;
                        return false;
                    }
                    has_version = true;
                } else if (strcmp(field_name, "version_fw") == 0) {
                    char versionFw[25];
                    strncpy(versionFw, decoded + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
                    versionFw[tokens[i + 1].end - tokens[i + 1].start] = '\0';

                    if (print.wifly) printf("[wifly] Firmware version: %s\n", versionFw);
                    int versionCheck = info_checkVersion(versionFw);
                    if (versionCheck < 0) {
                        if (versionCheck < -1) {
                            if (print.fbw) printf("[wifly] ERROR: version check failed!\n");
                            status = WIFLY_ERR_PARSE;
                        } else {
                            if (print.fbw) printf("[wifly] WARNING: a new pico-fbw firmware version is available, please download it!\n");
                            status = WIFLY_WARN_FW_VERSION;
                        }
                    }
                    has_version_fw = true;
                } else if (strcmp(field_name, "alt_samples") == 0) {
                    char altSamples[25];
                    strncpy(altSamples, decoded + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
                    altSamples[tokens[i + 1].end - tokens[i + 1].start] = '\0';
                    numAltSamples = atoi(altSamples);

                    // We expect a value between zero and 100 (only calculate if non-zero)
                    if (numAltSamples <= 100 && numAltSamples >= 0) {
                        if (print.wifly) printf("[wifly] Num alt samples: %d\n", numAltSamples);
                        if (numAltSamples != 0) {
                            if (status == WIFLY_STATUS_OK) status = WIFLY_STATUS_GPS_OFFSET; // Only replace the status if it is still OK (no warnings yet)
                        }
                    } else {
                        status = WIFLY_ERR_PARSE;
                        return false;
                    }
                    has_alt_samples = true;
                } else if (strcmp(field_name, "waypoints") == 0) {
                    if (tokens[i + 1].type == JSMN_ARRAY) {
                        waypoint_count = tokens[i + 1].size;
                        if (print.wifly) printf("[wifly] Flightplan contains %d waypoints\n", waypoint_count);
                        // Allocate memory for the waypoints array
                        waypoints = calloc(waypoint_count, sizeof(Waypoint));
                        if (!waypoints) {
                            if (print.fbw) printf("[wifly] ERROR: memory allocation for waypoint data failed!\n");
                            fplanStatus = WIFLY_ERR_MEM;
                            return false;
                        }
                        uint waypoint_token_index = i + 2; // Skip the array token
                        // Waypoint iteration
                        for (uint w = 0; w < waypoint_count; w++) {
                            if (print.wifly) printf("Processing waypoint %d:\n", w + 1);
                            if (tokens[waypoint_token_index].type == JSMN_OBJECT) {
                                uint waypoint_token_count = tokens[waypoint_token_index].size;
                                uint waypoint_field_token_index = waypoint_token_index + 1; // Skip the object token
                                Waypoint waypoint = {
                                    .lat = INFINITY,
                                    .lng = INFINITY,
                                    .alt = INT16_MIN,
                                    .speed = INFINITY,
                                    .drop = 0,
                                };
                                // Waypoint field iteration
                                for (uint j = 0; j < waypoint_token_count; j++) {
                                    if (tokens[waypoint_field_token_index].type == JSMN_STRING) {
                                        char waypoint_field_name[5];
                                        strncpy(waypoint_field_name, decoded + tokens[waypoint_field_token_index].start,
                                                tokens[waypoint_field_token_index].end - tokens[waypoint_field_token_index].start);
                                        waypoint_field_name[tokens[waypoint_field_token_index].end - tokens[waypoint_field_token_index].start] = '\0';
                                        
                                        if (strcmp(waypoint_field_name, "lat") == 0) {
                                            char lat[25];
                                            strncpy(lat, decoded + tokens[waypoint_field_token_index + 1].start,
                                                    tokens[waypoint_field_token_index + 1].end - tokens[waypoint_field_token_index + 1].start);
                                            lat[tokens[waypoint_field_token_index + 1].end - tokens[waypoint_field_token_index + 1].start] = '\0';

                                            // Store the latitude value into a Waypoint
                                            waypoint.lat = strtold(lat, NULL);
                                            if (print.wifly) printf("Latitude: %s\n", lat);
                                        } else if (strcmp(waypoint_field_name, "lng") == 0) {
                                            char lng[25];
                                            strncpy(lng, decoded + tokens[waypoint_field_token_index + 1].start,
                                                    tokens[waypoint_field_token_index + 1].end - tokens[waypoint_field_token_index + 1].start);
                                            lng[tokens[waypoint_field_token_index + 1].end - tokens[waypoint_field_token_index + 1].start] = '\0';
                                            
                                            waypoint.lng = strtold(lng, NULL);
                                            if (print.wifly) printf("Longitude: %s\n", lng);
                                        } else if (strcmp(waypoint_field_name, "alt") == 0) {
                                            char alt[4];
                                            strncpy(alt, decoded + tokens[waypoint_field_token_index + 1].start,
                                                    tokens[waypoint_field_token_index + 1].end - tokens[waypoint_field_token_index + 1].start);
                                            alt[tokens[waypoint_field_token_index + 1].end - tokens[waypoint_field_token_index + 1].start] = '\0';
                                            
                                            waypoint.alt = atoi(alt);
                                            if (print.wifly) printf("Altitude: %s\n", alt);
                                        } else if (strcmp(waypoint_field_name, "spd") == 0) {
                                            char spd[25];
                                            strncpy(spd, decoded + tokens[waypoint_field_token_index + 1].start,
                                                    tokens[waypoint_field_token_index + 1].end - tokens[waypoint_field_token_index + 1].start);
                                            spd[tokens[waypoint_field_token_index + 1].end - tokens[waypoint_field_token_index + 1].start] = '\0';

                                            waypoint.speed = strtof(spd, NULL);
                                            if (print.wifly) printf("Speed: %s\n", spd);
                                        } else if (strcmp(waypoint_field_name, "drp") == 0) {
                                            char drp[4];
                                            strncpy(drp, decoded + tokens[waypoint_field_token_index + 1].start,
                                                    tokens[waypoint_field_token_index + 1].end - tokens[waypoint_field_token_index + 1].start);
                                            drp[tokens[waypoint_field_token_index + 1].end - tokens[waypoint_field_token_index + 1].start] = '\0';
                                            
                                            waypoint.drop = atoi(drp);
                                            if (print.wifly) printf("Drop: %s\n", drp);
                                        } else {
                                            if (print.fbw) printf("[wifly] ERROR: waypoint field name not recognized!\n");
                                            fplanStatus = WIFLY_ERR_PARSE;
                                            return false;
                                        }
                                        // Advance to the next field
                                        waypoint_field_token_index += 2;
                                    }
                                }
                                // Check if the waypoint data is valid (aka that we got everything we expect and need)
                                if (waypoint.lat == INFINITY || waypoint.lng == INFINITY || waypoint.alt == INT16_MIN || waypoint.speed == INFINITY) {
                                    if (print.fbw) printf("[wifly] ERROR: waypoint data is invalid!\n");
                                    fplanStatus = WIFLY_ERR_PARSE;
                                    return false;
                                }
                                // Store the parsed waypoint into the master array
                                waypoints[w] = waypoint;
                                // Advance to the next waypoint
                                waypoint_token_index += WAYPOINT_NUM_FIELDS * 2 + 1; // + 1 for object token yet again
                            } else {
                                if (print.fbw) printf("[wifly] ERROR: waypoint token type not recognized!\n");
                                fplanStatus = WIFLY_ERR_PARSE;
                                return false;
                            }
                        }
                    } else {
                        if (print.fbw) printf("[wifly] ERROR: waypoints array not found!\n");
                        fplanStatus = WIFLY_ERR_PARSE;
                        return false;
                    }
                }
            }
        }
        if (!has_version || !has_version_fw || !has_alt_samples) {
            if (print.wifly) printf("[wifly] ERROR: flightplan missing data!\n");
            fplanStatus = WIFLY_ERR_PARSE;
            return false;
        }
        
        if (print.wifly) printf("[wifly] Waypoint data:\n");
        for (int i = 0; i < waypoint_count; i++) {
            if (print.wifly) printf("Waypoint #%d: lat=%.10f, lng=%.10f, alt=%d, speed=%f, drop=%d\n",
                                    i + 1, waypoints[i].lat, waypoints[i].lng, waypoints[i].alt, waypoints[i].speed, waypoints[i].drop);
        }
        // Save the flightplan JSON string
        fplanJson = malloc(strlen(decoded) + 1);
        if (!fplanJson) {
            if (print.fbw) printf("[wifly] ERROR: memory allocation for flightplan JSON failed!\n");
            fplanStatus = WIFLY_ERR_MEM;
            return false;
        }
        strcpy(fplanJson, decoded);
        fplanStatus = status;
        if (fplanStatus == WIFLY_STATUS_OK || fplanStatus == WIFLY_STATUS_GPS_OFFSET || fplanStatus == WIFLY_WARN_FW_VERSION) {
            log_message(INFO, "Flightplan recieved!", -1, 0, false);
            return true;
        } else {
            return false;
        }
    } else return true;
}

bool wifly_fplanExists() { return waypoints && waypoint_count > 0; }

Waypoint *wifly_getFplan() { return waypoints; }

const char *wifly_getFplanJson() { return fplanJson; }

uint wifly_getWaypointCount() { return waypoint_count; }

int wifly_getNumAltSamples() { return numAltSamples; }
