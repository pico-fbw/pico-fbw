/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
*/

#include <stdio.h>

#include "../../../info.h"

#include "help.h"

void api_help(const char *cmd, const char *args) {
    printf("\npico-fbw API v%s\n"
           "Commands:\n"
           "GET_CONFIG <section> <key> - Get configuration value (providing no arguments will list the entire config)\n"
           "GET_FLASH - Dump the flash contents used by pico-fbw\n"
           "GET_FPLAN - Get flightplan JSON from Wi-Fly\n"
           "GET_INFO - Get system information\n"
           "GET_INPUT - Get current control inputs\n"
           "GET_LOGS - Get system logs\n"
           "GET_MODE - Get the current flight mode\n"
           "GET_SENSOR <0/1/2> - Get sensor data <0=all, 1=AAHRS only, 2=GPS only>\n"
           "GET_THRUST - Get current thrust value\n"
           "SET_BAY <0/1> - Set the current position of the drop bay <0=closed, 1=open>"
           "SET_CONFIG <section> <key> <value> [-S] - Set configuration value (providing no arguments will save config to flash)\n"
           "SET_FPLAN <json> - Set the flightplan JSON\n"
           "SET_MODE <mode> - Set the flight mode\n"
           "SET_TARGET <roll> <pitch> <yaw> [thrust] - Set the desired attitude/thrust target in normal mode\n"
           "SET_WAYPOINT <lat> <lng> [alt] [speed] [drop] - Create and track onto a waypoint with the desired parameters in auto mode\n"
           "TEST_ALL - Runs all possible system tests using default values\n"
           "TEST_AAHRS - Tests the AAHRS (Altitude-Attitude Heading Reference System)\n"
           "TEST_GPS - Tests the GPS module (must be outside with good sky visibility)\n"
           "TEST_PWM [in_1] [out_1] [in_2] [out_2] [in_3] [out_3] [in_4] [out_4] [in_5] [out_5] - Tests the PWM system, the user must connect the specified pins\n"
           "TEST_SERVO [servo] - Tests the servo on the specified pin (or all enabled servos if none are specified)\n"
           "TEST_THROTTLE [time_idle] [time_mct] [time_max] - Tests the system's throttle in all three configured detents\n"
           "ABOUT - Display system information\n"
           "HELP - Display this help message\n"
           "PING - Pong!\n"
           "REBOOT <0/1> - Reboot the system (0=normal, 1=USB bootloader)\n"
           "RESET - Reset pico-fbw to factory defaults\n\n"
           "Responses:\n"
           "200 OK - Request successful\n"
           "202 Accepted - Request successful, but not complete yet\n"
           "204 No Content - Request successful, but no content available to return\n"
           "400 Bad Request - Invalid request format or parameters\n"
           "403 Forbidden - Request not allowed in the current state\n"
           "404 Unknown Command - Command not found\n"
           "500 Internal Error - Internal error executing the requested command\n\n"
           "More information can be found at https://pico-fbw.org/wiki/docs/API",
           PICO_FBW_API_VERSION);
}
