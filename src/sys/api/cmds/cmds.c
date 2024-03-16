/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include <string.h>

#include "GET/get_config.h"
#include "GET/get_fplan.h"
#include "GET/get_info.h"
#include "GET/get_input.h"
#include "GET/get_logs.h"
#include "GET/get_mode.h"
#include "GET/get_sensor.h"

#include "SET/set_bay.h"
#include "SET/set_config.h"
#include "SET/set_fplan.h"
#include "SET/set_mode.h"
#include "SET/set_target.h"
#include "SET/set_waypoint.h"

#include "TEST/test_aahrs.h"
#include "TEST/test_all.h"
#include "TEST/test_gps.h"
#include "TEST/test_pwm.h"
#include "TEST/test_servo.h"
#include "TEST/test_throttle.h"

#include "MISC/about.h"
#include "MISC/help.h"
#include "MISC/ping.h"
#include "MISC/reboot.h"
#include "MISC/reset.h"

#include "cmds.h"

i32 api_handle_get(const char *cmd, const char *args) {
    if (strcasecmp(cmd, "GET_CONFIG") == 0) {
        return api_get_config(args);
    } else if (strcasecmp(cmd, "GET_FPLAN") == 0) {
        return api_get_fplan(args);
    } else if (strcasecmp(cmd, "GET_INFO") == 0) {
        return api_get_info(args);
    } else if (strcasecmp(cmd, "GET_INPUT") == 0) {
        return api_get_input(args);
    } else if (strcasecmp(cmd, "GET_LOGS") == 0) {
        return api_get_logs(args);
    } else if (strcasecmp(cmd, "GET_MODE") == 0) {
        return api_get_mode(args);
    } else if (strcasecmp(cmd, "GET_SENSOR") == 0) {
        return api_get_sensor(args);
    } else
        return 404;
}

i32 api_handle_set(const char *cmd, const char *args) {
    if (strcasecmp(cmd, "SET_BAY") == 0) {
        return api_set_bay(args);
    } else if (strcasecmp(cmd, "SET_CONFIG") == 0) {
        return api_set_config(args);
    } else if (strcasecmp(cmd, "SET_FPLAN") == 0) {
        return api_set_fplan(args);
    } else if (strcasecmp(cmd, "SET_MODE") == 0) {
        return api_set_mode(args);
    } else if (strcasecmp(cmd, "SET_TARGET") == 0) {
        return api_set_target(args);
    } else if (strcasecmp(cmd, "SET_WAYPOINT") == 0) {
        return api_set_waypoint(args);
    } else
        return 404;
}

i32 api_handle_test(const char *cmd, const char *args) {
    if (strcasecmp(cmd, "TEST_AAHRS") == 0) {
        return api_test_aahrs(args);
    } else if (strcasecmp(cmd, "TEST_ALL") == 0) {
        return api_test_all(args);
    } else if (strcasecmp(cmd, "TEST_GPS") == 0) {
        return api_test_gps(args);
    } else if (strcasecmp(cmd, "TEST_PWM") == 0) {
        return api_test_pwm(args);
    } else if (strcasecmp(cmd, "TEST_SERVO") == 0) {
        return api_test_servo(args);
    } else if (strcasecmp(cmd, "TEST_THROTTLE") == 0) {
        return api_test_throttle(args);
    } else
        return 404;
}

i32 api_handle_misc(const char *cmd, const char *args) {
    if (strcasecmp(cmd, "ABOUT") == 0) {
        api_about(args);
    } else if (strcasecmp(cmd, "HELP") == 0) {
        api_help(args);
    } else if (strcasecmp(cmd, "PING") == 0) {
        api_ping(args);
    } else if (strcasecmp(cmd, "REBOOT") == 0) {
        api_reboot(args);
    } else if (strcasecmp(cmd, "RESET") == 0) {
        api_reset(args);
    } else
        return 404;
    return -1;
}
