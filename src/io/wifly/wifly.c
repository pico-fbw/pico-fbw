/**
 * Huge thank-you to Rasperry Pi (as a part of the Pico examples library) for providing much of the code used in Wi-Fly!
 * 
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
*/

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "../../lib/jsmn.h"

#include "pcl/dhcp.h"
#include "pcl/dns.h"
#include "pcl/tcp.h"

#include "wifly.h"
#include "../../config.h"

dhcp_server_t dhcp_server;
dns_server_t dns_server;

void wifly_init() {
    TCP_SERVER_T *state = calloc(1, sizeof(TCP_SERVER_T));
    if (!state) {
        ERROR_printf("failed to allocate state\n");
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

    // Start the dhcp server
    dhcp_server_init(&dhcp_server, &state->gw, &mask);
    // Start the dns server
    dns_server_init(&dns_server, &state->gw);

    if (!tcp_server_open(state)) {
        ERROR_printf("failed to open server\n");
    }
}

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

int wifly_parseFplan(const char *fplan) {
    // Find the start of the JSON string
    const char *json_start = strstr(fplan, FPLAN_PARAM);
    if (!json_start) {
        // Prefix not found
        return WIFLY_ERROR_PARSE;
    }
    // Move the pointer to the start of the JSON string
    // This effectively skips the "fplan=" prefix so we don't confuse the JSON parser later
    json_start += strlen(FPLAN_PARAM);

    // Now, URL decode the input string so we can parse it in JSON format instead of URL
    char decoded[strlen(json_start) + 1];
    strcpy(decoded, json_start);
    url_decode(decoded);
    WIFLY_DEBUG_printf("\n\nFlightplan data encoded: %s\n", json_start);
    WIFLY_DEBUG_printf("\nFlightplan data decoded: %s\n\n", decoded);

    // Initialize JSON parsing logic
    jsmn_parser parser;
    jsmntok_t tokens[strlen(json_start) + 1];
    jsmn_init(&parser);
    int token_count = jsmn_parse(&parser, decoded, strlen(decoded), tokens, sizeof(tokens)/sizeof(tokens[0]));
    if (token_count < 0) {
        return WIFLY_ERROR_PARSE;
    }

    for (int i = 0; i < token_count; i++) {
        if (tokens[i].type == JSMN_STRING) {
            // Get the field name
            char field_name[20];
            strncpy(field_name, decoded + tokens[i].start, tokens[i].end - tokens[i].start);
            field_name[tokens[i].end - tokens[i].start] = '\0';
            // Process the field based on its name
            if (strcmp(field_name, "version") == 0) {
                // Process the version
                char version[9];
                strncpy(version, decoded + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
                version[tokens[i + 1].end - tokens[i + 1].start] = '\0';
                WIFLY_DEBUG_printf("Flightplan version: %s\n", version);
                if (strcmp(version, WIFLY_CURRENT_VERSION) != 0) {
                    return WIFLY_ERROR_VERSION;
                }
            } else if (strcmp(field_name, "waypoints") == 0) {
                // Process the waypoints array
                if (tokens[i + 1].type == JSMN_ARRAY) {
                    int waypoint_count = tokens[i + 1].size;
                    WIFLY_DEBUG_printf("Flightplan contains %d waypoints\n", waypoint_count);
                    // Process each waypoint
                    for (int w = 0; w < waypoint_count; w++) {
                        WIFLY_DEBUG_printf("Waypoint %d:\n", w + 1);
                        // TODO: actually extract each waypoint's data I couldn't get it to work lol
                    }
                } else {
                    return WIFLY_ERROR_PARSE;
                }
            }
        }
    }
    return WIFLY_STATUS_OK;
}

void wifly_deinit() {
    dns_server_deinit(&dns_server);
    dhcp_server_deinit(&dhcp_server);
    // cyw43_arch_deinit();
}    
