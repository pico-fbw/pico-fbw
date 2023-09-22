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
#include "pico/runtime.h"
#include "pico/stdio.h"
#include "pico/time.h"
#include "pico/types.h"
#include "../lib/jsmn.h"

#include "flash.h"
#include "platform.h"
#include "servo.h"

#include "../modes/flight.h"
#include "../modes/modes.h"
#include "../modes/normal.h"

#include "../wifly/wifly.h"

#include "../sys/info.h"

#include "api.h"

#ifdef API_ENABLED

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
        int c = getchar_timeout_us(API_CHAR_TIMEOUT_US);
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

/**
 * Handles API GET commands.
 * @param cmd the command.
 * @param args the command arguments.
 * @return the status code.
*/
static uint api_handle_get(const char *cmd, const char *args) {
    // TODO: GET-ERRORS command
    if (strcmp(cmd, "GET_FLASH") == 0) {
        printf("{\"sectors\":[{\"values\":[{");
        // For float sectors, check for finite values and change them to null because json is dumb
        for (FloatSector s = FLOAT_SECTOR_MIN; s <= FLOAT_SECTOR_MAX; s++) {
            if (s != FLOAT_SECTOR_MAX) {
                for (uint v = 0; v <= (FLOAT_SECTOR_SIZE - 1); v++) {
                    float value = flash_readFloat(s, v);
                    if (v != (FLOAT_SECTOR_SIZE - 1)) {
                        if (isfinite(value)) {
                            printf("\"%d\":%f,", v, value);
                        } else {
                            printf("\"%d\":null,", v);
                        }
                    } else {
                        if (isfinite(value)) {
                            printf("\"%d\":%f},{", v, value);
                        } else {
                            printf("\"%d\":null},{", v);
                        }
                    }
                }
            } else {
                for (uint v = 0; v <= (FLOAT_SECTOR_SIZE - 1); v++) {
                    float value = flash_readFloat(FLOAT_SECTOR_MAX, v);
                    if (v != (FLOAT_SECTOR_SIZE - 1)) {
                        if (isfinite(value)) {
                            printf("\"%d\":%f,", v, value);
                        } else {
                            printf("\"%d\":null,", v);
                        }
                    } else {
                        if (isfinite(value)) {
                            printf("\"%d\":%f}]}]}\n", v, value);
                        } else {
                            printf("\"%d\":null}]}]}\n", v);
                        }
                    }
                }
            }
        }
        // Now handle string sectors...
        printf("{\"sectors\":[");
        for (StringSector s = STRING_SECTOR_MIN; s <= STRING_SECTOR_MAX; s++) {
            const char *value = flash_readString(s);
            if (s != STRING_SECTOR_MAX) {
                if (value != NULL) {
                    printf("\"%s\",", value);
                } else {
                    printf("\"\",");
                }
            } else {
                if (value != NULL) {
                    printf("\"%s\"]}\n", value);
                } else {
                    printf("\"\"]}\n");
                }
            }
        }
        return 200;
    } else if (strcmp(cmd, "GET_FPLAN") == 0) {
        const char *fplan = wifly_getFplanJson();
        if (fplan != NULL && wifly_getWaypointCount() > 0) {
            printf("%s\n", fplan);
            return 200;
        } else {
            return 503;
        }
    } else if (strcmp(cmd, "GET_GPS") == 0) {
        #ifdef GPS_ENABLED
            if (getCurrentMode() != MODE_DIRECT) {
                printf("{\"gps\":[{\"lat\":%f,\"lng\":%f,\"alt\":%d,\"spd\":%f}]}\n", gps.lat, gps.lng, gps.alt, gps.spd);
                return 200;
            } else {
                return 503;
            }
        #else
            return 501;
        #endif
    } else if (strcmp(cmd, "GET_IMU") == 0) {
        if (getCurrentMode() != MODE_DIRECT) {
            printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}]}\n", aircraft.roll, aircraft.pitch, aircraft.yaw);
            return 200;
        } else {
            return 503;
        }
    } else if (strcmp(cmd, "GET_INFO") == 0) {
        #ifdef RASPBERRYPI_PICO
            printf("{\"version\":\"%s\",\"version_api\":\"%s\",\"version_wifly\":\"\",\"is_pico_w\":false,\"rp2040_chip_version\":%d,\"rp2040_rom_version\":%d}\n",
                PICO_FBW_VERSION, PICO_FBW_API_VERSION, rp2040_chip_version(), (rp2040_rom_version() - 1));
        #endif
        #ifdef RASPBERRYPI_PICO_W
            printf("{\"version\":\"%s\",\"version_api\":\"%s\",\"version_wifly\":\"%s\",\"is_pico_w\":false,\"rp2040_chip_version\":%d,\"rp2040_rom_version\":%d}\n",
                PICO_FBW_VERSION, PICO_FBW_API_VERSION, WIFLY_VERSION, rp2040_chip_version(), (rp2040_rom_version() - 1));
        #endif
        return 200;
    } else if (strcmp(cmd, "GET_MODE") == 0) {
        printf("{\"mode\":%d}\n", getCurrentMode());
        return 200;
    } else if (strcmp(cmd, "GET_PID") == 0) {
        printf("{\"roll\":{\"p\":");
        // In case you were wondering json is still dumb
        if (isfinite(flash_readFloat(FLOAT_SECTOR_PID, 1))) {
            printf("%f", flash_readFloat(FLOAT_SECTOR_PID, 1));
        } else {
            printf("null");
        }
        printf(",\"i\":");
        if (isfinite(flash_readFloat(FLOAT_SECTOR_PID, 2))) {
            printf("%f", flash_readFloat(FLOAT_SECTOR_PID, 2));
        } else {
            printf("null");
        }
        printf(",\"d\":");
        if (isfinite(flash_readFloat(FLOAT_SECTOR_PID, 3))) {
            printf("%f", flash_readFloat(FLOAT_SECTOR_PID, 3));
        } else {
            printf("null");
        }
        printf("},\"pitch\":{\"p\":");
        if (isfinite(flash_readFloat(FLOAT_SECTOR_PID, 4))) {
            printf("%f", flash_readFloat(FLOAT_SECTOR_PID, 4));
        } else {
            printf("null");
        }
        printf(",\"i\":");
        if (isfinite(flash_readFloat(FLOAT_SECTOR_PID, 5))) {
            printf("%f", flash_readFloat(FLOAT_SECTOR_PID, 5));
        } else {
            printf("null");
        }
        printf(",\"d\":");
        if (isfinite(flash_readFloat(FLOAT_SECTOR_PID, 6))) {
            printf("%f", flash_readFloat(FLOAT_SECTOR_PID, 6));
        } else {
            printf("null");
        }
        printf("}}\n");
        return 200;
    } else if (strcmp(cmd, "GET_SENSORS") == 0) {
        if (getCurrentMode() != MODE_DIRECT) {
            #ifdef GPS_ENABLED
                printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}],\"gps\":[{\"lat\":%f,\"lng\":%f,\"alt\":%d,\"spd\":%f}]}\n", aircraft.roll, aircraft.pitch, aircraft.yaw, gps.lat, gps.lng, gps.alt, gps.spd);
            #else
                printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}]}\n", aircraft.roll, aircraft.pitch, aircraft.yaw);
            #endif
            return 200;
        } else {
            return 503;
        }
    } else if (strcmp(cmd, "GET_THRUST") == 0) {
        return 501; // Unsupported for now
    } else {
        return 404;
    }
}

/**
 * Handles API SET commands.
 * @param cmd the command.
 * @param args the command arguments.
 * @return the status code.
*/
static uint api_handle_set(const char *cmd, const char *args) {
    // SET_FPLAN is parsed seperately and SET_MODE / SET_THRUST are one-argument commands, so they do not require a parser
    if (strcmp(cmd, "SET_FPLAN") == 0) {
        char *fplan = malloc(strlen(args) + 7);
        if (fplan != NULL) {
            // Format as an HTTP request for the parser
            sprintf(fplan, FPLAN_PARAM_CONCAT, args);
            if (wifly_parseFplan(fplan)) {
                free(fplan);
                return 200;
            } else {
                free(fplan);
                return 500;
            }
        } else {
            return 500;
        }
    } else if (strcmp(cmd, "SET_MODE") == 0) {
        Mode mode = atoi(args);
        // Ensure mode is valid before setting it
        if (mode >= MODE_MIN && mode <= MODE_MAX) {
            toMode(mode);
            return 200;
        } else {
            return 400;
        }
    } else if (strcmp(cmd, "SET_THRUST") == 0) {
        return 501; // TODO: implement once athr lib is complete
    } else {
        // All other SET commands utilize JSON so we should initatiate the JSON parser (documented in wifly.c if you're curious)
        bool goodRequest = false; // Used to determine if the JSON was parsed correctly; without there would be no output entirely with a bad request
        uint status = 400; // Bad request by default
        jsmn_parser parser;
        jsmntok_t tokens[strlen(args)];
        jsmn_init(&parser);
        int token_count = jsmn_parse(&parser, args, strlen(args), tokens, sizeof(tokens)/sizeof(tokens[0]));
        if (token_count > 0) {
            if (strcmp(cmd, "SET_PID") == 0) {
                float rollP, rollI, rollD, pitchP, pitchI, pitchD = -255.0f;
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
                    float pid[FLOAT_SECTOR_SIZE] = {FLAG_PID, rollP, rollI, rollD, pitchP, pitchI, pitchD};
                    flash_writeFloat(FLOAT_SECTOR_PID, pid);
                    status = 200;
                    goodRequest = true;
                }
            } else if (strcmp(cmd, "SET_SETPOINTS") == 0) {
                if (getCurrentMode() == MODE_NORMAL) {
                    float rollSet, pitchSet, yawSet = -255.0f;
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
                    if ((rollSet != -255.0f && pitchSet != -255.0f && yawSet != -255.0f) && (rollSet < ROLL_LIMIT_HOLD && rollSet > -ROLL_LIMIT_HOLD && pitchSet < PITCH_UPPER_LIMIT && pitchSet > PITCH_LOWER_LIMIT && yawSet < MAX_RUD_DEFLECTION && yawSet > -MAX_RUD_DEFLECTION)) {
                        if (mode_normalSetSetpoints(rollSet, pitchSet, yawSet)) {
                            status = 200;
                            goodRequest = true;
                        } else {
                            status = 423;
                            goodRequest = true;
                        }
                    }
                } else {
                    status = 403;
                    goodRequest = true;
                }
            } else {
                return 404;
            }
            return status;
        } else {
            return 400;
        }
    }
}

/**
 * Handles API TEST commands.
 * @param cmd the command.
 * @param args the command arguments.
 * @return the status code.
*/
static uint api_handle_test(const char *cmd, const char *args) {
    // TODO: tests for PWM, IMU, and GPS
    if (strcmp(cmd, "TEST_SERVO") == 0) {
        uint num_servos;
        const uint16_t degrees[] = DEFAULT_SERVO_TEST;
        if (args == NULL) {
            // No arguments, test with default values
            const uint servos[] = SERVO_PINS;
            num_servos = NUM_SERVOS;
            servo_test(servos, num_servos, degrees, NUM_DEFAULT_SERVO_TEST, DEFAULT_SERVO_TEST_PAUSE_MS);
        } else {
            // Test the servo that was provided in the command
            const uint servos[] = {atoi(args)};
            num_servos = 1;
            servo_test(servos, num_servos, degrees, NUM_DEFAULT_SERVO_TEST, DEFAULT_SERVO_TEST_PAUSE_MS);
        }
        return 200;
    } else {
        return 404;
    }
}

/**
 * Handles API CONFIG commands.
 * @param cmd the command.
 * @param args the command arguments.
 * @return the status code.
*/
static uint api_handle_config(const char *cmd, const char *args) {

}

/**
 * Handles API REBOOT commands.
 * @param cmd the command.
 * @param args the command arguments.
 * @return the status code.
*/
static uint api_handle_reboot(const char *cmd, const char *args) {
    // TODO: REBOOT and REBOOT_BOOTSEL commands
}

void api_poll() {
    if (time_us_64() > ENABLE_API_TIMEOUT_MS * 1000) {
        char *line = stdin_read_line();
        // Check if there has been input
        if (line != NULL) {
            if (strlen(line) < 1) {
                free(line);
                return;
            }
            // Seperate the command and arguments
            char *rcmd = strtok(line, " ");
            char *args = strtok(NULL, " ");
            
            char *cmd = malloc(strlen(rcmd) + 1);
            if (cmd != NULL) {
                char *p = cmd;
                while (*rcmd) {
                    *p = toupper((unsigned char)*rcmd);
                    p++;
                    rcmd++;
                }
                *p = '\0';
            } else {
                // Out of memory?
                printf("pico-fbw 500\n");
                free(line);
                return;
            }
            // Command handler
            uint status = 500; // Default to internal error
            if (strncmp(cmd, "GET_", 4) == 0) {
                status = api_handle_get(cmd, args);
            } else if (strncmp(cmd, "SET_", 4) == 0) {
                status = api_handle_set(cmd, args);
            } else if (strncmp(cmd, "TEST_", 5) == 0) {
                status = api_handle_test(cmd, args);
            // Non-prefix (misc) commands
            } else if (strcmp(cmd, "ABOUT") == 0) {
                #if defined(RASPBERRYPI_PICO)
                    printf("pico-fbw v%s, API v%s, Wi-Fly Unsupported, RP2040-B%d, ROM RP2040-B%d\n\n",
                           PICO_FBW_VERSION, PICO_FBW_API_VERSION, rp2040_chip_version(), (rp2040_rom_version() - 1));
                #elif defined(RASPBERRYPI_PICO_W)
                    printf("pico(w)-fbw v%s, API v%s, Wi-Fly v%s, RP2040-B%d, ROM RP2040-B%d\n\n",
                           PICO_FBW_VERSION, PICO_FBW_API_VERSION, WIFLY_VERSION, rp2040_chip_version(), (rp2040_rom_version() - 1));
                #endif
                printf("Copyright (C) 2023 pico-fbw\n\n"
                       "This program is free software: you can redistribute it and/or modify"
                       "it under the terms of the GNU General Public License as published by"
                       "the Free Software Foundation, either version 3 of the License, or"
                       "(at your option) any later version.\n\n"

                       "This program is distributed in the hope that it will be useful,"
                       "but WITHOUT ANY WARRANTY; without even the implied warranty of"
                       "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the"
                       "GNU General Public License for more details.\n\n"

                       "You should have received a copy of the GNU General Public License"
                       "along with this program. If not, see <https://www.gnu.org/licenses/>.\n");
                return;
            } else if (strcmp(cmd, "HELP") == 0) {
                printf("\npico-fbw API v%s\n"
                       "Commands:\n"
                       "GET_FLASH - Dump the flash contents used by pico-fbw\n"
                       "GET_FPLAN - Get flightplan JSON from Wi-Fly\n"
                       "GET_GPS - Get GPS data\n"
                       "GET_IMU - Get IMU data\n"
                       "GET_INFO - Get system information\n"
                       "GET_MODE - Get the current flight mode (%d-%d)\n"
                       "GET_PID - Get PID constants\n"
                       "GET_SENSORS - Get sensor data\n"
                       "GET_THRUST - Get current thrust value\n"
                       "SET_FPLAN <flight_plan> - Set the flightplan JSON\n"
                       "SET_MODE <mode> - Set the flight mode\n"
                       "SET_PID {\"roll\":{\"p\":<roll_p>,\"i\":<roll_i>,\"d\":<roll_d>},\"pitch\":{\"p\":<pitch_p>,\"i\":<pitch_i>,\"d\":<pitch_d>}} - Set PID constants\n"
                       "SET_SETPOINTS {\"roll\":<roll>,\"pitch\":<pitch>,\"yaw\":<yaw>} - Set the desired attitude setpoints in normal mode\n"
                       "SET_THRUST <thrust> - Set the thrust value\n"
                       "TEST_SERVO <servo> Tests the servo (or all enabled servos if none is specified)\n"
                       "ABOUT - Display system information\n"
                       "HELP - Display this help message\n"
                       "PING - Pong!\n\n"
                       "Responses:\n"
                       "200 OK - Request successful\n"
                       "400 Bad Request - Invalid request format or parameters\n"
                       "403 Forbidden - Request not allowed in the current state\n"
                       "404 Unknown Command - Command not found\n"
                       "423 Locked - Changes are not allowed in the current state\n"
                       "500 Internal Error - Internal error executing the requested command\n"
                       "501 Not Implemented - Command not implemented\n"
                       "503 Unavailable - Request temporarily unavailable\n",
                       PICO_FBW_API_VERSION, MODE_MIN, MODE_MAX);
                return;
            } else if (strcmp(cmd, "PING") == 0) {
                printf("PONG\n");
                return;
            } else {
                status = 404; // No match
            }
            // Print the status code from earlier command execution
            if (status != 404) {
                printf("pico-fbw %d\n", status);
            } else {
                printf("pico-fbw 404\n");
            }
            free(cmd);
            free(line);
        }
    }
}

#endif // API_ENABLED
