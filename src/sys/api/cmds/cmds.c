/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
*/

#include <string.h>
#include "pico/types.h"

#include "sys/api/cmds/GET/get_config.h"
#include "sys/api/cmds/GET/get_flash.h"
#include "sys/api/cmds/GET/get_fplan.h"
#include "sys/api/cmds/GET/get_info.h"
#include "sys/api/cmds/GET/get_input.h"
#include "sys/api/cmds/GET/get_logs.h"
#include "sys/api/cmds/GET/get_mode.h"
#include "sys/api/cmds/GET/get_sensor.h"

#include "sys/api/cmds/SET/set_bay.h"
#include "sys/api/cmds/SET/set_config.h"
#include "sys/api/cmds/SET/set_fplan.h"
#include "sys/api/cmds/SET/set_mode.h"
#include "sys/api/cmds/SET/set_target.h"
#include "sys/api/cmds/SET/set_waypoint.h"

#include "sys/api/cmds/TEST/test_all.h"
#include "sys/api/cmds/TEST/test_aahrs.h"
#include "sys/api/cmds/TEST/test_gps.h"
#include "sys/api/cmds/TEST/test_pwm.h"
#include "sys/api/cmds/TEST/test_servo.h"
#include "sys/api/cmds/TEST/test_throttle.h"

#include "sys/api/cmds/MISC/about.h"
#include "sys/api/cmds/MISC/help.h"
#include "sys/api/cmds/MISC/ping.h"
#include "sys/api/cmds/MISC/reboot.h"
#include "sys/api/cmds/MISC/reset.h"

#include "sys/api/cmds/cmds.h"

int api_handle_get(const char *cmd, const char *args) {
    if (strcasecmp(cmd, "GET_CONFIG") == 0) {
        return api_get_config(cmd, args);
    } else if (strcasecmp(cmd, "GET_FLASH") == 0) {
        return api_get_flash(cmd, args);
    } else if (strcasecmp(cmd, "GET_FPLAN") == 0) {
        return api_get_fplan(cmd, args);
    } else if (strcasecmp(cmd, "GET_INFO") == 0) {
        return api_get_info(cmd, args);
    } else if (strcasecmp(cmd, "GET_INPUT") == 0) {
        return api_get_input(cmd, args);
    } else if (strcasecmp(cmd, "GET_LOGS") == 0) {
        return api_get_logs(cmd, args);
    } else if (strcasecmp(cmd, "GET_MODE") == 0) {
        return api_get_mode(cmd, args);
    } else if (strcasecmp(cmd, "GET_SENSOR") == 0) {
        return api_get_sensor(cmd, args);
    } else return 404;
}

int api_handle_set(const char *cmd, const char *args) {
    if (strcasecmp(cmd, "SET_BAY") == 0) {
        return api_set_bay(cmd, args);
    } else if (strcasecmp(cmd, "SET_CONFIG") == 0) {
        return api_set_config(cmd, args);
    } else if (strcasecmp(cmd, "SET_FPLAN") == 0) {
        return api_set_fplan(cmd, args);
    } else if (strcasecmp(cmd, "SET_MODE") == 0) {
        return api_set_mode(cmd, args);
    } else if (strcasecmp(cmd, "SET_TARGET") == 0) {
        return api_set_target(cmd, args);
    } else if (strcasecmp(cmd, "SET_WAYPOINT") == 0) {
        return api_set_waypoint(cmd, args);
    } else return 404;
}

int api_handle_test(const char *cmd, const char *args) {
    if (strcasecmp(cmd, "TEST_AAHRS") == 0) {
        return api_test_aahrs(cmd, args);
    } else if (strcasecmp(cmd, "TEST_ALL") == 0) {
        return api_test_all(cmd, args);
    } else if (strcasecmp(cmd, "TEST_GPS") == 0) {
        return api_test_gps(cmd, args);
    } else if (strcasecmp(cmd, "TEST_PWM") == 0) {
        return api_test_pwm(cmd, args);
    } else if (strcasecmp(cmd, "TEST_SERVO") == 0) {
        return api_test_servo(cmd, args);
    } else if (strcasecmp(cmd, "TEST_THROTTLE") == 0) {
        return api_test_throttle(cmd, args);
    } else return 404;
}

int api_handle_misc(const char *cmd, const char *args) {
    if (strcasecmp(cmd, "ABOUT") == 0) {
        api_about(cmd, args);
    } else if (strcasecmp(cmd, "HELP") == 0) {
        api_help(cmd, args);
    } else if (strcasecmp(cmd, "PING") == 0) {
        api_ping(cmd, args);
    } else if (strcasecmp(cmd, "REBOOT") == 0) {
        api_reboot(cmd, args);
    } else if (strcasecmp(cmd, "RESET") == 0) {
        api_reset(cmd, args);
    } else return 404;
    return -1;
}
