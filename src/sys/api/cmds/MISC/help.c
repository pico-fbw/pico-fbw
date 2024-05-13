/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "sys/print.h"
#include "sys/version.h"

#include "help.h"

i32 api_help(const char *args) {
    printraw("\npico-fbw API v%s\n"
             "Commands:\n"
             "GET_CONFIG - Get system configuration value(s)\n"
             "GET_FLIGHTPLAN - Get raw flightplan data\n"
             "GET_INFO - Get system information\n"
             "GET_INPUT - Get current control inputs\n"
             "GET_LOGS - Get system logs\n"
             "GET_MODE - Get the current flight mode\n"
             "GET_SENSOR - Get sensor data\n"
             "SET_BAY - Set the current position of the drop bay\n"
             "SET_CONFIG - Set system configuration value(s)\n"
             "SET_FLIGHTPLAN - Set raw flightplan data\n"
             "SET_MODE - Set the current flight mode\n"
             "SET_TARGET - Set the desired attitude/thrust target\n"
             "SET_WAYPOINT - Create and track onto a Waypoint\n"
             "TEST_ALL - Runs all possible system tests using default values\n"
             "TEST_AAHRS - Tests the AAHRS (Altitude-Attitude Heading Reference System)\n"
             "TEST_GPS - Tests the GPS module\n"
             "TEST_PWM - Tests the PWM input system\n"
             "TEST_SERVO - Tests the servo(s)\n"
             "TEST_THROTTLE - Tests the throttle\n"
             "ABOUT - Display system information\n"
             "HELP - Display this help message\n"
             "PING - Pong!\n"
             "REBOOT - Reboot the system\n"
             "RESET - Reset pico-fbw to \"factory\" defaults\n"
             "\nMore information can be found at https://pico-fbw.org/wiki/docs/API\n",
             PICO_FBW_API_VERSION);
    return -1;
    (void)args;
}
