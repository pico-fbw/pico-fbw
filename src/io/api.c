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
#include "pico/config.h"
#include "pico/platform.h"
#include "pico/runtime.h"
#include "pico/stdio.h"
#include "pico/time.h"
#include "pico/types.h"

#include "hardware/watchdog.h"

#include "flash.h"
#include "platform.h"
#include "servo.h"

#include "../lib/jsmn.h"

#include "../modes/flight.h"
#include "../modes/modes.h"
#include "../modes/normal.h"

#include "../sys/config.h"
#include "../sys/info.h"

#include "../wifly/wifly.h"

#include "api.h"

#define API_CHAR_TIMEOUT_US 100 // Timeout when waiting for a character to arrive

/**
 * Reads a line from stdin if available.
 * @return A pointer to the line read if there was one (automatically null-terminated),
 *         or NULL if there was no input available.
 * @note This function does not free the memory allocated for the line if read, ensure to free() it after use.
*/
static inline char *stdin_read_line() {
    char *buf = NULL;
    uint i = 0;
    while (true) {
        int c = getchar_timeout_us(API_CHAR_TIMEOUT_US);
        if (c == '\n' || c == '\r' || c == PICO_ERROR_TIMEOUT) {
            break;
        } else {
            // Otherwise, store character in buffer and increment
            buf = (char*)realloc(buf, (i + 1) * sizeof(char));
            buf[i] = c;
            i++;
        }
    }
    // Null-terminate the buffer if we read a line, otherwise return NULL
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
    if (strcasecmp(cmd, "GET_CONFIG") == 0) {
        if (args) {
            char section[64];
            char key[64];
            if (sscanf(args, "%63s %63s", section, key) < 2) return 400;
            switch (config_getSectionType(section)) {
                case SECTION_TYPE_FLOAT: {
                    float k = config_getFloat(section, key);
                    if (k != infinityf()) {
                        printf("{\"key\":%f}\n", k);
                    } else {
                        return 400;
                    }
                    break;
                }
                case SECTION_TYPE_STRING: {
                    const char *k = config_getString(section, key);
                    if (k) {
                        printf("{\"key\":\"%s\"}\n", k);
                    } else {
                        return 400;
                    }
                    break;
                }
            }
        } else {
            // No args, gather all data
            float configFloat[NUM_FLOAT_CONFIG_VALUES];
            config_getAllFloats(configFloat, NUM_FLOAT_CONFIG_VALUES);
            const char *configStr[NUM_STRING_CONFIG_VALUES];
            config_getAllStrings(configStr, NUM_STRING_CONFIG_VALUES);
            printf("{\"sections\":[");
            for (ConfigSectionIndex s = 0; s < NUM_CONFIG_SECTIONS; s++) {
                const char *sectionStr = config_sectionToString(s);
                printf("{\"name\":\"%s\",\"keys\":[", sectionStr);
                switch (config_getSectionType(sectionStr)) {
                    case SECTION_TYPE_FLOAT: {
                        if (s != NUM_CONFIG_SECTIONS - 1) {
                            for (uint v = 0; v < NUM_FLOAT_VALUES_PER_SECTION; v++) {
                                float value = configFloat[s * NUM_FLOAT_VALUES_PER_SECTION + v];
                                if (v != NUM_FLOAT_VALUES_PER_SECTION - 1) {
                                    // For float sectors, check for finite values and change them to null because json is dumb
                                    isfinite(value) ? printf("%f,", value) : printf("null,");
                                } else {
                                    isfinite(value) ? printf("%f]},", value) : printf("null]},");
                                }
                            }
                        } else {
                            for (uint v = 0; v < NUM_FLOAT_VALUES_PER_SECTION; v++) {
                                float value = configFloat[s * NUM_FLOAT_VALUES_PER_SECTION + v];
                                if (v != NUM_FLOAT_VALUES_PER_SECTION - 1) {
                                    isfinite(value) ? printf("%f,", value) : printf("null,");
                                } else {
                                    isfinite(value) ? printf("%f]}]}\n", value) : printf("null]}]}\n");
                                }
                            }
                        }
                        break;
                    }
                    case SECTION_TYPE_STRING: {
                        for (uint v = 0; v < NUM_STRING_VALUES_PER_SECTION; v++) {
                            const char *value = configStr[v];
                            if (v != NUM_STRING_VALUES_PER_SECTION - 1) {
                                value ? printf("\"%s\",", value) : printf("\"\",");
                            } else {
                                value ? printf("\"%s\"]}]}\n", value) : printf("\"\"]}]}\n");
                            }
                        }
                    }
                }
            }
        }
        return 200;
    } else if (strcasecmp(cmd, "GET_FLASH") == 0) {
        printf("{\"sectors\":[");
        for (FloatSector s = FLOAT_SECTOR_MIN; s <= FLOAT_SECTOR_MAX; s++) {
            printf("{\"values\":[");
            for (uint v = 0; v <= (FLOAT_SECTOR_SIZE - 1); v++) {
                float value = flash_readFloat(s, v);
                if (v != (FLOAT_SECTOR_SIZE - 1)) {
                    isfinite(value) ? printf("%f,", value) : printf("null,");
                } else {
                    isfinite(value) ? printf("%f]},", value) : printf("null]},");
                }
            }
        }
        for (StringSector s = STRING_SECTOR_MIN; s <= STRING_SECTOR_MAX; s++) {
            const char *value = flash_readString(s);
            if (s != STRING_SECTOR_MAX) {
                value ? printf("\"%s\",", value) : printf("\"\",");
            } else {
                value ? printf("\"%s\"]}\n", value) : printf("\"\"]}\n");
            }
        }
        return 200;
    } else if (strcasecmp(cmd, "GET_FPLAN") == 0) {
        const char *fplan = wifly_getFplanJson();
        if (fplan != NULL && wifly_getWaypointCount() > 0) {
            printf("%s\n", fplan);
            return 200;
        } else {
            return 403;
        }
    } else if (strcasecmp(cmd, "GET_INFO") == 0) {
        #if defined(RASPBERRYPI_PICO)
            printf("{\"version\":\"%s\",\"version_api\":\"%s\",\"version_wifly\":\"\",\"is_pico_w\":false,\"rp2040_version\":%d}\n",
                PICO_FBW_VERSION, PICO_FBW_API_VERSION, rp2040_chip_version());
        #elif defined(RASPBERRYPI_PICO_W)
            printf("{\"version\":\"%s\",\"version_api\":\"%s\",\"version_wifly\":\"%s\",\"is_pico_w\":true,\"rp2040_version\":%d}\n",
                PICO_FBW_VERSION, PICO_FBW_API_VERSION, WIFLY_VERSION, rp2040_chip_version());
        #endif
        return 200;
    } else if (strcasecmp(cmd, "GET_MODE") == 0) {
        printf("{\"mode\":%d}\n", getCurrentMode());
        return 200;
    } else if (strcasecmp(cmd, "GET_PID") == 0) {
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
    } else if (strcasecmp(cmd, "GET_SENSOR") == 0) {
        // Sensors are not update in direct mode
        if (getCurrentMode() != MODE_DIRECT) {
            switch (atoi(args)) {
                case 1: // IMU only
                    printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}]}\n", aircraft.roll, aircraft.pitch, aircraft.yaw);
                    break;
                case 2: // GPS only
                    if (config.sensors.gpsEnabled) {
                        printf("{\"gps\":[{\"lat\":%f,\"lng\":%f,\"alt\":%d,\"spd\":%f,\"trk\":%f}]}\n",
                        gps.lat, gps.lng, gps.alt, gps.spd, gps.trk_true);
                    } else {
                        return 501;
                    }
                    break;
                case 0: // All sensors
                default:
                    if (config.sensors.gpsEnabled) {
                        printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}],\"gps\":[{\"lat\":%f,\"lng\":%f,\"alt\":%d,\"spd\":%f,\"trk\":%f}]}\n",
                        aircraft.roll, aircraft.pitch, aircraft.yaw, gps.lat, gps.lng, gps.alt, gps.spd, gps.trk_true);
                    } else {
                        printf("{\"imu\":[{\"roll\":%.4f,\"pitch\":%.4f,\"yaw\":%.4f}]}\n", aircraft.roll, aircraft.pitch, aircraft.yaw);
                    }
                    break;
            }
            return 200;
        } else {
            return 403;
        }
    } else if (strcasecmp(cmd, "GET_THRUST") == 0) {
        return 501; // Unsupported for now
    } else {
        return 404;
    }
    return 500; // Nothing else returned (shouldn't happen)
}

/**
 * Handles API SET commands.
 * @param cmd the command.
 * @param args the command arguments.
 * @return the status code.
*/
static uint api_handle_set(const char *cmd, const char *args) {
    if (strcasecmp(cmd, "SET_CONFIG") == 0) {
        if (args) {
            char section[64];
            char key[64];
            char value[64];
            char params[16];
            if (sscanf(args, "%63s %63s %63s %15s", section, key, value, params) < 3) return 400;
            switch (config_getSectionType(section)) {
                case SECTION_TYPE_FLOAT:
                    config_setFloat(section, key, atoff(value));
                    break;
                case SECTION_TYPE_STRING:
                    config_setString(section, key, value);
                    break;
            }
            // Save to flash immediately if requested
            if (strncasecmp(params, "-S", 2) == 0) {
                if (!config_save()) return 400;
            }
        } else {
            if (!config_save()) return 400; // No args, trigger a save to flash
        }
        return 200;
    } else if (strcasecmp(cmd, "SET_FPLAN") == 0) {
        char *fplan = malloc(strlen(args) + 7);
        if (fplan != NULL) {
            // Format as an HTTP request for the parser
            sprintf(fplan, FPLAN_PARAM_CONCAT, args);
            bool status = wifly_parseFplan(fplan);
            free(fplan);
            if (status) {
                return 200;
            } else {
                return 500;
            }
        } else {
            return 500;
        }
    } else if (strcasecmp(cmd, "SET_MODE") == 0) {
        Mode mode = atoi(args);
        // Ensure mode is valid before setting it
        if (mode >= MODE_MIN && mode <= MODE_MAX) {
            toMode(mode);
            return 200;
        } else {
            return 400;
        }
    } else if (strcasecmp(cmd, "SET_PID") == 0) {
        float pid[FLOAT_SECTOR_SIZE] = {FLAG_PID};
        // Parse arguments for paremeters, verify number of arguments and that they were positive, and write to flash
        if ((sscanf(args, "%f %f %f %f %f %f", &pid[1], &pid[2], &pid[3], &pid[4], &pid[5], &pid[6]) >= 6) &&
        pid[1] > 0 && pid[2] > 0 && pid[3] > 0 && pid[4] > 0 && pid[5] > 0 && pid[6] > 0) {
            flash_writeFloat(FLOAT_SECTOR_PID, pid);
            return 200;
        } else {
            return 400;
        }
    } else if (strcasecmp(cmd, "SET_SETPOINTS") == 0) {
        if (getCurrentMode() == MODE_NORMAL) {
            float roll, pitch, yaw;
            if ((sscanf(args, "%f %f %f", &roll, &pitch, &yaw) >= 3) &&
            (roll > config.limits.rollLimitHold || roll < -config.limits.rollLimitHold) &&
            (pitch > config.limits.pitchUpperLimit || pitch < config.limits.pitchLowerLimit) &&
            (yaw > config.limits.maxRudDeflection || yaw < -config.limits.maxRudDeflection)) {
                if (mode_normalSetSetpoints(roll, pitch, yaw)) {
                    return 200;
                } else {
                    return 423; // Rejected by normal mode, user probably inputting already
                }
            }
        } else {
            return 403;
        }
    } else if (strcasecmp(cmd, "SET_THRUST") == 0) {
        return 501; // TODO: implement once athr lib is complete
    } else {
        return 404;
    }
    return 500;
}

/**
 * Handles API TEST commands.
 * @param cmd the command.
 * @param args the command arguments.
 * @return the status code.
*/
static uint api_handle_test(const char *cmd, const char *args) {
    // TODO: tests for FLASH, PWM, IMU, GPS, and ALL (diagnostic)
    if (strcasecmp(cmd, "TEST_SERVO") == 0) {
        if (getCurrentMode() == MODE_DIRECT) {
            uint num_servos = 3;
            uint servos[num_servos];
            const uint16_t degrees[] = DEFAULT_SERVO_TEST;
            if (args) {
                // Test the servo that was provided in the command
                servos[0] = atoi(args);
                num_servos = 1;
                servo_test(servos, num_servos, degrees, NUM_DEFAULT_SERVO_TEST, DEFAULT_SERVO_TEST_PAUSE_MS);
            } else {
                // No arguments given, test with config values
                servo_getPins(servos, &num_servos);
                servo_test(servos, num_servos, degrees, NUM_DEFAULT_SERVO_TEST, DEFAULT_SERVO_TEST_PAUSE_MS);
            }
        } else {
            return 403;
        }
        return 200;
    } else {
        return 404;
    }
    return 500;
}

void api_poll() {
    char *line = stdin_read_line();
    // Check if there has been input
    if (line != NULL) {
        if (strlen(line) < 1) {
            free(line);
            return;
        }
        // Seperate the command and arguments
        char *cmd = strtok(line, " ");
        char *args = strtok(NULL, "");
        if (cmd == NULL) {
            // Out of memory?
            printf("pico-fbw 500\n");
            free(line);
            return;
        }

        // Command handler
        uint status = 500; // Default to internal error
        if (strncasecmp(cmd, "GET_", 4) == 0) {
            status = api_handle_get(cmd, args);
        } else if (strncasecmp(cmd, "SET_", 4) == 0) {
            status = api_handle_set(cmd, args);
        } else if (strncasecmp(cmd, "TEST_", 5) == 0) {
            status = api_handle_test(cmd, args);
        // Non-prefix (misc) commands
        } else if (strcasecmp(cmd, "ABOUT") == 0) {
            switch (platform_type()) {
                case PLATFORM_FBW:
                    printf("Genuine pico-fbw v%s, API v%s, Wi-Fly v%s, RP2040-B%d\n\n",
                            PICO_FBW_VERSION, PICO_FBW_API_VERSION, WIFLY_VERSION, rp2040_chip_version());
                    break;
                case PLATFORM_PICO_W:
                    printf("pico(w)-fbw v%s, API v%s, Wi-Fly v%s, RP2040-B%d\n\n",
                            PICO_FBW_VERSION, PICO_FBW_API_VERSION, WIFLY_VERSION, rp2040_chip_version());
                    break;
                default:
                    // Generic message for unknown platforms and regular Pico
                    printf("pico-fbw v%s, API v%s, Wi-Fly Unsupported, RP2040-B%d\n\n",
                            PICO_FBW_VERSION, PICO_FBW_API_VERSION, rp2040_chip_version());
                    break;
            }
            printf("Copyright (C) 2023 pico-fbw\n\n"
                    "This program is free software: you can redistribute it and/or modify "
                    "it under the terms of the GNU General Public License as published by "
                    "the Free Software Foundation, either version 3 of the License, or "
                    "(at your option) any later version.\n\n"

                    "This program is distributed in the hope that it will be useful, "
                    "but WITHOUT ANY WARRANTY; without even the implied warranty of "
                    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
                    "GNU General Public License for more details.\n\n"

                    "You should have received a copy of the GNU General Public License "
                    "along with this program. If not, see https://www.gnu.org/licenses/.\n");
            return;
        } else if (strcasecmp(cmd, "HELP") == 0) {
            printf("\npico-fbw API v%s\n"
                    "Commands:\n"
                    "GET_CONFIG <section> <key> - Get configuration value\n"
                    "GET_FLASH - Dump the flash contents used by pico-fbw\n"
                    "GET_FPLAN - Get flightplan JSON from Wi-Fly\n"
                    "GET_INFO - Get system information\n"
                    "GET_MODE - Get the current flight mode (%d-%d)\n"
                    "GET_PID - Get PID constants\n"
                    "GET_SENSOR <0/1/2> - Get sensor data (0=all, 1=IMU only, 2=GPS only)\n"
                    "GET_THRUST - Get current thrust value\n"
                    "SET_FPLAN <flight_plan> - Set the flightplan JSON\n"
                    "SET_MODE <mode> - Set the flight mode\n"
                    "SET_PID <roll_p> <roll_i> <roll_d> <pitch_p> <pitch_i> <pitch_d> - Set PID constants\n"
                    "SET_SETPOINTS <roll> <pitch> <yaw>} - Set the desired attitude setpoints in normal mode\n"
                    "SET_THRUST <thrust> - Set the thrust value\n"
                    "TEST_SERVO <servo> Tests the servo (or all enabled servos if none is specified)\n"
                    "ABOUT - Display system information\n"
                    "HELP - Display this help message\n"
                    "PING - Pong!\n"
                    "REBOOT <0/1> - Reboot the system (0=normal, 1=USB bootloader)\n"
                    "RESET - Reset the device to factory defaults\n\n"
                    "Responses:\n"
                    "200 OK - Request successful\n"
                    "400 Bad Request - Invalid request format or parameters\n"
                    "403 Forbidden - Request not allowed in the current state\n"
                    "404 Unknown Command - Command not found\n"
                    "423 Locked - Changes are not allowed in the current state\n"
                    "500 Internal Error - Internal error executing the requested command\n"
                    "501 Not Implemented - Command not implemented\n",
                    PICO_FBW_API_VERSION, MODE_MIN, MODE_MAX);
            return;
        } else if (strcasecmp(cmd, "PING") == 0) {
            printf("PONG\n");
            return;
        } else if (strcasecmp(cmd, "REBOOT") == 0) {
            switch (atoi(args)) {
                case 1:
                    platform_reboot(REBOOT_BOOTLOADER);
                case 0:
                default:
                    platform_reboot(REBOOT_FAST);
            }
        } else if (strcasecmp(cmd, "RESET") == 0) {
            if (strncasecmp(args, "-F", 2) != 0) {
                printf("This will erase ALL data stored on the device! Are you sure? (y/n)\n");
                char *ans = NULL;
                while (!ans) {
                    ans = stdin_read_line();
                    watchdog_update();
                }
                if (strcasecmp(ans, "Y") != 0) {
                    printf("Reset cancelled.\n");
                    free(ans);
                    free(line);
                    return;
                }
            }
            printf("Reset will begin shortly...\n");
            sleep_ms(2500);
            flash_reset();
            printf("Reset complete. Shutting down...\n");
            platform_shutdown();
        } else {
            status = 404; // No match
        }
        // Print the status code from earlier command execution
        if (status != 404) {
            printf("pico-fbw %d\n", status);
        } else {
            printf("pico-fbw 404\n");
        }
        free(line);
    }
}
