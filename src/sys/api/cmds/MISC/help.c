/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>
#include "pico/types.h"

#include "../../../../modes/modes.h"

#include "../../../info.h"

#include "help.h"

void api_help(const char *cmd, const char *args) {
    printf("\npico-fbw API v%s\n"
           "Commands:\n"
           "GET_CONFIG <section> <key> - Get configuration value\n"
           "GET_FLASH - Dump the flash contents used by pico-fbw\n"
           "GET_FPLAN - Get flightplan JSON from Wi-Fly\n"
           "GET_INFO - Get system information\n"
           "GET_LOGS - Get system logs\n"
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
           "204 No Content - Request successful, but no content available to return\n"
           "400 Bad Request - Invalid request format or parameters\n"
           "403 Forbidden - Request not allowed in the current state\n"
           "404 Unknown Command - Command not found\n"
           "423 Locked - Changes are not allowed in the current state\n"
           "500 Internal Error - Internal error executing the requested command\n"
           "501 Not Implemented - Command not implemented\n",
           PICO_FBW_API_VERSION, MODE_MIN, MODE_MAX);
}
