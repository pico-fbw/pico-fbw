/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "sys/api/api.h"
#include "sys/print.h"

#include "cmds.h"

/**
 * Wraps an API command handler for use with stdin/stdout.
 * @param args the command arguments from stdin
 * @param handler the handler function to call
 * @param has_output whether the command is expected to produce output
 * @return the status code of the operation
 */
static i32 api_wrap_handler(const char *args, api_handler handler, bool has_output) {
    char *out = NULL;
    i32 res = handler(args, &out);
    if (res != 200 || (has_output && !out)) {
        if (out) {
            free(out);
        }
        return res;
    }
    printraw("%s\n", out);
    free(out);
    return -1; // -1 indicates the same as 200, but indicates that output has already been printed
}

i32 api_handle_get(const char *cmd, const char *args) {
    if (strcasecmp(cmd, "GET_CONFIG") == 0) {
        return api_wrap_handler(args, api_get_config, true);
    } else if (strcasecmp(cmd, "GET_FLIGHTPLAN") == 0) {
        return api_wrap_handler(args, api_get_flightplan, true);
    } else if (strcasecmp(cmd, "GET_INFO") == 0) {
        return api_wrap_handler(args, api_get_info, true);
    } else if (strcasecmp(cmd, "GET_INPUT") == 0) {
        return api_wrap_handler(args, api_get_input, true);
    } else if (strcasecmp(cmd, "GET_LOGS") == 0) {
        return api_wrap_handler(args, api_get_logs, true);
    } else if (strcasecmp(cmd, "GET_MODE") == 0) {
        return api_wrap_handler(args, api_get_mode, true);
    } else if (strcasecmp(cmd, "GET_SENSOR") == 0) {
        return api_wrap_handler(args, api_get_sensor, true);
    } else {
        return 404;
    }
}

i32 api_handle_set(const char *cmd, const char *args) {
    if (strcasecmp(cmd, "SET_BAY") == 0) {
        return api_wrap_handler(args, api_set_bay, false);
    } else if (strcasecmp(cmd, "SET_CONFIG") == 0) {
        return api_wrap_handler(args, api_set_config, true);
    } else if (strcasecmp(cmd, "SET_FLIGHTPLAN") == 0) {
        return api_wrap_handler(args, api_set_flightplan, true);
    } else if (strcasecmp(cmd, "SET_MODE") == 0) {
        return api_wrap_handler(args, api_set_mode, false);
    } else if (strcasecmp(cmd, "SET_TARGET") == 0) {
        return api_wrap_handler(args, api_set_target, false);
    } else if (strcasecmp(cmd, "SET_WAYPOINT") == 0) {
        return api_wrap_handler(args, api_set_waypoint, false);
    } else {
        return 404;
    }
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
    } else {
        return 404;
    }
}

i32 api_handle_misc(const char *cmd, const char *args) {
    if (strcasecmp(cmd, "ABOUT") == 0) {
        return api_about(args);
    } else if (strcasecmp(cmd, "HELP") == 0) {
        return api_help(args);
    } else if (strcasecmp(cmd, "PING") == 0) {
        return api_ping(args);
    } else if (strcasecmp(cmd, "REBOOT") == 0) {
        return api_reboot(args);
    } else if (strcasecmp(cmd, "RESET") == 0) {
        return api_reset(args);
    } else {
        return 404;
    }
}
