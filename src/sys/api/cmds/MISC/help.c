/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/defs.h"

#include "sys/print.h"
#include "sys/version.h"

#include "help.h"

i32 api_help(const char *args) {
    printraw(
        "\npico-fbw API v%s\n"
        "Commands:\n"
        "GET_CONFIG <section> <key> - Get configuration value (no arguments: list the entire config)\n"
        "GET_FPLAN - Get raw flightplan data\n"
        "GET_INFO - Get system information\n"
        "GET_INPUT - Get current control inputs\n"
        "GET_LOGS - Get system logs\n"
        "GET_MODE - Get the current flight mode\n"
#if PLATFORM_SUPPORTS_ADC
        "GET_SENSOR <0/1/2/3> - Get sensor data <0/no arguments=all, 1=AAHRS, 2=GPS, 3=battery>\n"
#else
        "GET_SENSOR <0/1/2> - Get sensor data <0=all, 1=AAHRS, 2=GPS>\n"
#endif
        "GET_THRUST - Get current thrust value\n"
        "SET_BAY <0/1> - Set the current position of the drop bay <0=closed, 1=open>\n"
        "SET_CONFIG <section> <key> <value> [-S] - Set configuration value (no arguments: save current config to flash) [-S = "
        "save]\n"
        "SET_FPLAN <json> - Set the flightplan JSON\n"
        "SET_MODE <mode> - Set the flight mode\n"
        "SET_TARGET <roll> <pitch> <yaw> [thrust] - Set the desired attitude/thrust target in normal mode\n"
        "SET_WAYPOINT <lat> <lng> [alt] [speed] [drop] - Create and track onto a Waypoint with the desired parameters in auto "
        "mode\n"
        "TEST_ALL - Runs all possible system tests using default values\n"
        "TEST_AAHRS - Tests the AAHRS (Altitude-Attitude Heading Reference System)\n"
        "TEST_GPS - Tests the GPS module (must be outside with good sky visibility)\n"
        "TEST_PWM [in_1] [out_1] [in_2] [out_2] [in_3] [out_3] [in_4] [out_4] [in_5] [out_5] - Tests the PWM system, the user "
        "must connect the specified pins\n"
        "TEST_SERVO <servo> - Tests the servo on the specified pin (no arguments: all enabled servos)\n"
        "TEST_THROTTLE [time_idle] [time_mct] [time_max] - Tests the system's throttle in all three configured detents\n"
        "ABOUT - Display system information\n"
        "HELP - Display this help message\n"
        "PING - Pong!\n"
        "REBOOT <0/1> - Reboot the system (0/no arguments=normal, 1=bootloader, if present)\n"
        "RESET - Reset pico-fbw to factory defaults\n\n"
        "Responses:\n"
        "200 OK - Request successful\n"
        "202 Accepted - Request successful, but not complete yet\n"
        "204 No Content - Request successful, but no content available to return\n"
        "400 Bad Request - Invalid request format or parameters\n"
        "403 Forbidden - Request not allowed in the current state\n"
        "404 Unknown Command - Command not found\n"
        "500 Internal Error - Internal error executing the requested command\n\n"
        "More information can be found at https://pico-fbw.org/wiki/docs/API\n",
        PICO_FBW_API_VERSION);
    return -1;
    (void)args;
}
