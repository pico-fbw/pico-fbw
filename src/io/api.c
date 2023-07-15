/**
 * Source file of pico-fbw: https://github.com/MylesAndMore/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "pico/platform.h"
#include "pico/stdio.h"
#include "pico/time.h"
#include "../lib/jsmn.h"

#include "flash.h"

#include "../modes/flight.h"
#include "../modes/modes.h"
#include "../modes/normal.h"

#include "wifly/wifly.h"

#include "../version.h"

#include "api.h"

#ifdef API_ENABLED

static absolute_time_t apiEnabledTimeout;
static bool apiEnabled;

/**
 * Reads a line from stdin if available.
 * @return A pointer to the line read if there was one (automatically null-terminated),
 *         NULL if there was no input available.
 * @note This function does not free the memory allocated for the line if read, ensure to free() it after use.
*/
static inline char *stdin_read_line() {
    char *buf = NULL;
    uint i = 0;
    // Loop through the line until a new line character is found
    while (true) {
        // Get a character from stdin--0 means the function will simply return if there are no characters in stdin and NOT BLOCK!!
        int c = getchar_timeout_us(0);
        // Check if timeout or end of line were reached
        if (c == PICO_ERROR_TIMEOUT || c == '\n' || c == '\r') {
            break;
        } else {
            // Otherwise, store character in buffer and increment
            buf = (char*)realloc(buf, (i + 1) * sizeof(char));
            buf[i] = c;
            i++;
        }
    }
    // Null-terminate the buffer if we have read a line, otherwise return NULL
    if (i != 0) {
        buf = (char*)realloc(buf, (i + 1) * sizeof(char));
        buf[i] = '\0';
    }
    return buf;
}

void api_init_blocking() {
    apiEnabledTimeout = make_timeout_time_ms(ENABLE_API_TIMEOUT_MS);
    #ifdef API_WAIT_ON_BOOT
        printf("[api] waiting for PING...\n");
        while (true) {
            char *cmd = stdin_read_line();
            if (strcmp(cmd, "PING") == 0) {
                printf("PONG\n");
                free(cmd);
                break;
            }
        }
    #endif
}

void api_poll() {
    if (time_reached(apiEnabledTimeout)) {
        if (!apiEnabled) {
            printf("[api] ready!\n");
            apiEnabled = true;
        }
        char *line = stdin_read_line();
        // Check if there has been input
        if (line != NULL) {
            // Seperate the command and arguments
            char *rcmd = strtok(line, " ");
            char *args = strtok(NULL, " ");
            // Automatically uppercase the command
            char* cmd = malloc(strlen(rcmd) + 1);
            if (cmd != NULL) {
                char* p = cmd;
                while (*rcmd) {
                    *p = toupper((unsigned char)*rcmd);
                    p++;
                    rcmd++;
                }
                *p = '\0';
            }
            // Command handler:

            // All GET commands
            if (strncmp(cmd, "GET_", 4) == 0) {
                if (strcmp(cmd, "GET_MODE") == 0) {
                    printf("{\"mode\":%d}\n", getCurrentMode());
                    printf("pico-fbw 200 OK\n");
                } else if (strcmp(cmd, "GET_SENSORS") == 0) {
                    if (getCurrentMode() != DIRECT) {
                        #ifdef WIFLY_ENABLED
                            printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f,\"heading\":%.4f}],\"gps\":[{\"lat\":%f,\"lng\":%f,\"alt\":%d,\"spd\":%f}]}\n", aircraft.roll, aircraft.pitch, aircraft.yaw, aircraft.heading, gps.lat, gps.lng, gps.alt, gps.spd);
                        #else
                            printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f,\"heading\":%.4f}]}\n", aircraft.roll, aircraft.pitch, aircraft.yaw, aircraft.heading);
                        #endif
                        printf("pico-fbw 200 OK\n");
                    } else {
                        printf("pico-fbw 503 Unavailable\n");
                    }
                } else if (strcmp(cmd, "GET_IMU") == 0) {
                    if (getCurrentMode() != DIRECT) {
                        printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f,\"heading\":%.4f}]}\n", aircraft.roll, aircraft.pitch, aircraft.yaw, aircraft.heading);
                        printf("pico-fbw 200 OK\n");
                    } else {
                        printf("pico-fbw 503 Unavailable\n");
                    }
                } else if (strcmp(cmd, "GET_GPS") == 0) {
                    #ifdef WIFLY_ENABLED
                        if (getCurrentMode() != DIRECT) {
                            printf("{\"gps\":[{\"lat\":%f,\"lng\":%f,\"alt\":%d,\"spd\":%f}]}\n", gps.lat, gps.lng, gps.alt, gps.spd);
                            printf("pico-fbw 200 OK\n");
                        } else {
                            printf("pico-fbw 503 Unavailable\n");
                        }
                    #else
                        printf("pico-fbw 501 Not Implemented\n");
                    #endif
                } else if (strcmp(cmd, "GET_THRUST") == 0) {
                    // Unsupported for now
                    printf("pico-fbw 501 Not Implemented\n");
                } else if (strcmp(cmd, "GET_FPLAN") == 0) {
                    #ifdef WIFLY_ENABLED
                        const char *fplan = wifly_getFplanJson();
                        if (fplan == NULL || wifly_getWaypointCount() == 0) {
                            printf("pico-fbw 503 Unavailable\n");
                        } else {
                            printf("%s\n", fplan);
                            printf("pico-fbw 200 OK\n");
                        }
                    #else
                        printf("pico-fbw 501 Not Implemented\n");
                    #endif
                } else if (strcmp(cmd, "GET_PID") == 0) {
                    printf("{\"roll\":{\"p\":");
                    // Curtesy checking for -inf values and changing them to null because json is dumb
                    if (isfinite(flash_read(1, 1))) {
                        printf("%f", flash_read(1, 1));
                    } else {
                        printf("null");
                    }
                    printf(",\"i\":");
                    if (isfinite(flash_read(1, 2))) {
                        printf("%f", flash_read(1, 2));
                    } else {
                        printf("null");
                    }
                    printf(",\"d\":");
                    if (isfinite(flash_read(1, 3))) {
                        printf("%f", flash_read(1, 3));
                    } else {
                        printf("null");
                    }
                    printf("},\"pitch\":{\"p\":");
                    if (isfinite(flash_read(2, 1))) {
                        printf("%f", flash_read(2, 1));
                    } else {
                        printf("null");
                    }
                    printf(",\"i\":");
                    if (isfinite(flash_read(2, 2))) {
                        printf("%f", flash_read(2, 2));
                    } else {
                        printf("null");
                    }
                    printf(",\"d\":");
                    if (isfinite(flash_read(2, 3))) {
                        printf("%f", flash_read(2, 3));
                    } else {
                        printf("null");
                    }
                    printf("}}\npico-fbw 200 OK\n");
                } else if (strcmp(cmd, "GET_FLASH") == 0) {
                    printf("{\"sectors\":[{\"values\":[{");
                    // In case you were wondering json is still dumb
                    for (uint s = FLASH_MIN_SECTOR; s <= FLASH_MAX_SECTOR; s++) {
                        if (s != FLASH_MAX_SECTOR) {
                            for (uint v = FLASH_MIN_SECTOR; v <= 4; v++) {
                                if (v != 4) {
                                    if (isfinite(flash_read(s, v))) {
                                        printf("\"%d\":%f,", v, flash_read(s, v));
                                    } else {
                                        printf("\"%d\":null,", v);
                                    }
                                } else {
                                    if (isfinite(flash_read(s, v))) {
                                        printf("\"%d\":%f},{", v, flash_read(s, v));
                                    } else {
                                        printf("\"%d\":null},{", v);
                                    }
                                }
                            }
                        } else {
                            for (uint v = FLASH_MIN_SECTOR; v <= 4; v++) {
                                if (v != 4) {
                                    if (isfinite(flash_read(3, v))) {
                                        printf("\"%d\":%f,", v, flash_read(3, v));
                                    } else {
                                        printf("\"%d\":null},", v);
                                    }
                                } else {
                                    if (isfinite(flash_read(3, v))) {
                                        printf("\"%d\":%f}]}]}\n", v, flash_read(3, v));
                                    } else {
                                        printf("\"%d\":null}]}]}\n", v);
                                    }
                                }
                            }
                        }
                    }
                    printf("pico-fbw 200 OK\n");
                } else if (strcmp(cmd, "GET_INFO") == 0) {
                    #ifdef RASPBERRYPI_PICO
                        printf("{\"version\":\"%s\",\"version_api\":\"%s\",\"version_wifly\":\"Unsupported\",\"is_pico_w\":false,\"rp2040_chip_version\":%d,\"rp2040_rom_version\":%d}\n",
                            PICO_FBW_VERSION, PICO_FBW_API_VERSION, rp2040_chip_version(), (rp2040_rom_version() - 1));
                    #endif
                    #ifdef RASPBERRYPI_PICO_W
                        printf("{\"version\":\"%s\",\"version_api\":\"%s\",\"version_wifly\":\"%s\",\"is_pico_w\":false,\"rp2040_chip_version\":%d,\"rp2040_rom_version\":%d}\n",
                            PICO_FBW_VERSION, PICO_FBW_API_VERSION, WIFLY_VERSION, rp2040_chip_version(), (rp2040_rom_version() - 1));
                    #endif
                    printf("pico-fbw 200 OK\n");
                } else {
                    printf("pico-fbw 404 Unknown Command\n");
                }
            // All SET commands
            } else if (strncmp(cmd, "SET_", 4) == 0) {
                // SET_FPLAN command is a bit different (parsed in wifly.c, not here) so handle that one seperately
                if (strcmp(cmd, "SET_FPLAN") == 0) {
                    // TODO: allow some sort of auto mode on normal pico through this api command instead of through wifi?
                    #ifdef WIFLY_ENABLED
                        char *fplan = malloc(strlen(args) + 7);
                        if (fplan != NULL) {
                            // Automatically format as an HTTP request
                            sprintf(fplan, FPLAN_PARAM_CONCAT, args);
                            if (wifly_parseFplan(fplan)) {
                                printf("pico-fbw 200 OK\n");
                            } else {
                                printf("pico-fbw 500 Internal Error\n");
                            }
                            free(fplan);
                        } else {
                            printf("pico-fbw 500 Internal Error\n");
                        }
                    #else
                        printf("pico-fbw 501 Not Implemented\n");
                    #endif
                } else {
                    bool goodReq = false;
                    // All other SET commands utilize JSON so we should initatiate the JSON parser (all documented in wifly.c)
                    jsmn_parser parser;
                    jsmntok_t tokens[strlen(args)];
                    jsmn_init(&parser);
                    int token_count = jsmn_parse(&parser, args, strlen(args), tokens, sizeof(tokens)/sizeof(tokens[0]));
                    if (token_count > 0) {
                        if (strcmp(cmd, "SET_MODE") == 0) {
                            for (uint i = 0; i < token_count; i++) {
                                if (tokens[i].type == JSMN_STRING) {
                                    char field[25];
                                    strncpy(field, args + tokens[i].start, tokens[i].end - tokens[i].start);
                                    field[tokens[i].end - tokens[i].start] = '\0';
                                    if (strcmp(field, "mode") == 0) {
                                        char mode[2];
                                        strncpy(mode, args + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
                                        mode[tokens[i + 1].end - tokens[i + 1].start] = '\0';
                                        // Ensure mode is valid before setting it
                                        if (atoi(mode) >= DIRECT && atoi(mode) <= HOLD) {
                                            toMode(atoi(mode));
                                            printf("pico-fbw 200 OK\n");
                                            goodReq = true;
                                        }
                                    }
                                }
                            }
                        } else if (strcmp(cmd, "SET_SETPOINTS") == 0) {
                            if (getCurrentMode() == NORMAL) {
                                float rollSet, pitchSet, yawSet = -100.0f;
                                for (uint i = 0; i < token_count; i++) {
                                    if (tokens[i].type == JSMN_STRING) {
                                        char field[25];
                                        strncpy(field, args + tokens[i].start, tokens[i].end - tokens[i].start);
                                        field[tokens[i].end - tokens[i].start] = '\0';
                                        if (strcmp(field, "roll") == 0) {
                                            char roll[8];
                                            strncpy(roll, args + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
                                            roll[tokens[i + 1].end - tokens[i + 1].start] = '\0';
                                            rollSet = atof(roll);
                                        } else if (strcmp(field, "pitch") == 0) {
                                            char pitch[8];
                                            strncpy(pitch, args + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
                                            pitch[tokens[i + 1].end - tokens[i + 1].start] = '\0';
                                            pitchSet = atof(pitch);
                                        } else if (strcmp(field, "yaw") == 0) {
                                            char yaw[8];
                                            strncpy(yaw, args + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
                                            yaw[tokens[i + 1].end - tokens[i + 1].start] = '\0';
                                            yawSet = atof(yaw);
                                        }
                                    }
                                }
                                // World's longest if statement to make sure the setpoints are valid
                                if ((rollSet != -100.0f && pitchSet != -100.0f && yawSet != -100.0f) && (rollSet < ROLL_LIMIT_HOLD && rollSet > -ROLL_LIMIT_HOLD && pitchSet < PITCH_UPPER_LIMIT && pitchSet > PITCH_LOWER_LIMIT && yawSet < RUD_LIMIT && yawSet > -RUD_LIMIT)) {
                                    if (mode_normalSetSetpoints(rollSet, pitchSet, yawSet)) {
                                        printf("pico-fbw 200 OK\n");
                                        goodReq = true;
                                    } else {
                                        printf("pico-fbw 423 Locked\n");
                                        goodReq = true;
                                    }
                                }
                            } else {
                                printf("pico-fbw 403 Forbidden\n");
                                goodReq = true;
                            }
                        } else if (strcmp(cmd, "SET_THRUST") == 0) {
                            // Unsupported for now
                            printf("pico-fbw 501 Not Implemented\n");
                            goodReq = true;
                        } else if (strcmp(cmd, "SET_PID") == 0) {
                            float rollP, rollI, rollD, pitchP, pitchI, pitchD = -100.0f;
                            for (uint i = 0; i < token_count; i++) {
                                if (tokens[i].type == JSMN_STRING) {
                                    char field[25];
                                    strncpy(field, args + tokens[i].start, tokens[i].end - tokens[i].start);
                                    field[tokens[i].end - tokens[i].start] = '\0';
                                    if (strcmp(field, "roll") == 0) {
                                        if (tokens[i + 1].type == JSMN_OBJECT) {
                                            if (strncmp(args + tokens[i + 2].start, "p", tokens[i + 2].end - tokens[i + 2].start) == 0) {
                                                rollP = atof(args + tokens[i + 3].start);
                                            }
                                            if (strncmp(args + tokens[i + 4].start, "i", tokens[i + 4].end - tokens[i + 4].start) == 0) {
                                                rollI = atof(args + tokens[i + 5].start);
                                            }
                                            if (strncmp(args + tokens[i + 6].start, "d", tokens[i + 6].end - tokens[i + 6].start) == 0) {
                                                rollD = atof(args + tokens[i + 7].start);
                                            }
                                        }
                                    } else if (strcmp(field, "pitch") == 0) {
                                        if (tokens[i + 1].type == JSMN_OBJECT) {
                                            if (strncmp(args + tokens[i + 2].start, "p", tokens[i + 2].end - tokens[i + 2].start) == 0) {
                                                pitchP = atof(args + tokens[i + 3].start);
                                            }
                                            if (strncmp(args + tokens[i + 4].start, "i", tokens[i + 4].end - tokens[i + 4].start) == 0) {
                                                pitchI = atof(args + tokens[i + 5].start);
                                            }
                                            if (strncmp(args + tokens[i + 6].start, "d", tokens[i + 6].end - tokens[i + 6].start) == 0) {
                                                pitchD = atof(args + tokens[i + 7].start);
                                            }
                                        }
                                    }
                                }
                            }
                            if (rollP >= 0 && rollI >= 0 && rollD >= 0 && pitchP >= 0 && pitchI >= 0 && pitchD >= 0) {
                                goodReq = true;
                                // Get current flash data so we don't overwrite it (well technically we do overwrite it but write it back immediately after)
                                float pid0[CONFIG_SECTOR_SIZE];
                                for (uint v = 0; v < CONFIG_SECTOR_SIZE; v++) {
                                    pid0[v] = flash_read(FLASH_SECTOR_PID0, v);
                                }
                                pid0[0] = 0.3f;
                                pid0[1] = rollP;
                                pid0[2] = rollI;
                                pid0[3] = rollD;
                                float pid1[CONFIG_SECTOR_SIZE];
                                for (uint v = 0; v < CONFIG_SECTOR_SIZE; v++) {
                                    pid1[v] = flash_read(FLASH_SECTOR_PID1, v);
                                }
                                pid1[0] = 0.3f;
                                pid1[1] = pitchP;
                                pid1[2] = pitchI;
                                pid1[3] = pitchD;
                                // Write new value
                                flash_write(FLASH_SECTOR_PID0, pid0);
                                flash_write(FLASH_SECTOR_PID1, pid1);
                                printf("pico-fbw 200 OK\n");
                            }
                        } else if (strcmp(cmd, "SET_FLASH") == 0) {
                            uint sector;
                            uint index;
                            float value;
                            for (uint i = 0; i < token_count; i++) {
                                if (tokens[i].type == JSMN_STRING) {
                                    char field[25];
                                    strncpy(field, args + tokens[i].start, tokens[i].end - tokens[i].start);
                                    field[tokens[i].end - tokens[i].start] = '\0';
                                    if (strcmp(field, "sector") == 0) {
                                        char flsector[8];
                                        strncpy(flsector, args + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
                                        flsector[tokens[i + 1].end - tokens[i + 1].start] = '\0';
                                        // Ensure sector is valid
                                        sector = atoi(flsector);
                                        if (!(sector < FLASH_MIN_SECTOR || sector > FLASH_MAX_SECTOR || sector == FLASH_SECTOR_BOOT)) {
                                            goodReq = true;
                                        } else {
                                            goodReq = false;
                                            break;
                                        }
                                    } else if (strcmp(field, "index") == 0) {
                                        char flindex[8];
                                        strncpy(flindex, args + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
                                        flindex[tokens[i + 1].end - tokens[i + 1].start] = '\0';
                                        // Ensure index is valid
                                        index = atoi(flindex);
                                        if (!(index < 0 || index > CONFIG_SECTOR_SIZE)) {
                                            goodReq = true;
                                        } else {
                                            goodReq = false;
                                            break;
                                        }
                                    } else if (strcmp(field, "value") == 0) {
                                        char flvalue[8];
                                        strncpy(flvalue, args + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
                                        flvalue[tokens[i + 1].end - tokens[i + 1].start] = '\0';
                                        value = atof(flvalue);
                                    }
                                }
                            }
                            if (goodReq) {
                                float data[CONFIG_SECTOR_SIZE];
                                for (uint v = 0; v <= CONFIG_SECTOR_SIZE; v++) {
                                    data[v] = flash_read(sector, v);
                                }
                                data[index] = value;
                                flash_write(sector, data);
                                printf("pico-fbw 200 OK\n");
                            }
                        } else {
                            printf("pico-fbw 404 Unknown Command\n");
                            goodReq = true;
                        }
                        if (!goodReq) {
                            printf("pico-fbw 400 Bad Request\n");
                        }
                    } else {
                        printf("pico-fbw 400 Bad Request\n");
                    }
                }
            // Misc commands (no prefix)
            } else if (strcmp(cmd, "PING") == 0) {
                printf("PONG\n");
            } else if (strcmp(cmd, "HELP") == 0) {
                printf("\npico-fbw API v%s\n"
                       "Commands:\n"
                       "PING - Pong!\n"
                       "HELP - Display this help message\n"
                       "ABOUT - Display system information in plaintext\n"
                       "GET_MODE - Get the current flight mode (0-4)\n"
                       "GET_SENSORS - Get sensor data (IMU, GPS)\n"
                       "GET_IMU - Get IMU data\n"
                       "GET_GPS - Get GPS data\n"
                       "GET_THRUST - Get thrust value\n"
                       "GET_FPLAN - Get flightplan JSON from Wi-Fly\n"
                       "GET_PID - Get PID constants\n"
                       "GET_FLASH - Dump the flash contents used by pico-fbw\n"
                       "GET_INFO - Get system information\n"
                       "SET_MODE {\"mode\":<mode>} - Set the flight mode\n"
                       "SET_SETPOINTS {\"roll\":<roll>,\"pitch\":<pitch>,\"yaw\":<yaw>} - Set the desired attitude setpoints in normal mode\n"
                       "SET_THRUST {\"thrust\":<thrust>} - Set the thrust value\n"
                       "SET_FPLAN <flight_plan> - Set the flightplan JSON\n"
                       "SET_PID {\"roll\":{\"p\":<roll_p>,\"i\":<roll_i>,\"d\":<roll_d>},\"pitch\":{\"p\":<pitch_p>,\"i\":<pitch_i>,\"d\":<pitch_d>}} - Set PID constants\n"
                       "SET_FLASH {\"sector\":<sector>,\"index\",<index>,\"value\":<value>} - Write a single floating-point value to the flash\n"
                       "Responses:\n"
                       "200 OK - Request successful\n"
                       "400 Bad Request - Invalid request format or parameters\n"
                       "403 Forbidden - Request not allowed in the current state\n"
                       "404 Unknown Command - Command not found\n"
                       "423 Locked - Changes are not allowed in the current state\n"
                       "500 Internal Error - Internal error executing the requested command\n"
                       "501 Not Implemented - Command not implemented\n"
                       "503 Unavailable - Request temporarily unavailable\n",
                       PICO_FBW_API_VERSION);
            } else if (strcmp(cmd, "ABOUT") == 0) {
                #ifdef RASPBERRYPI_PICO
                    printf("pico-fbw v%s, API v%s, Wi-Fly Unsupported, RP2040-B%d, ROM RP2040-B%d\n", PICO_FBW_VERSION, PICO_FBW_API_VERSION, rp2040_chip_version(), (rp2040_rom_version() - 1));
                #endif
                #ifdef RASPBERRYPI_PICO_W
                    printf("pico-fbw(w) v%s, API v%s, Wi-Fly v%s, RP2040-B%d, ROM RP2040-B%d\n", PICO_FBW_VERSION, PICO_FBW_API_VERSION, WIFLY_VERSION, rp2040_chip_version(), (rp2040_rom_version() - 1));
                #endif
            } else if (strcmp(cmd, "PARMESEAN_PARTY") == 0) {
                printf("pico-fbw 1022 Party!\n");
            } else {
                printf("pico-fbw 404 Unknown Command\n");
            }
            free(cmd);
            free(line);
            // TODO: add commands for getting and setting (including wiping) PID settings
        }
    }
}

#endif // API_ENABLED
