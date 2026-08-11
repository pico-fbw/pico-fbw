/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the MIT License
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "sys/api/api.h"

#include "cmds.h"

/**
 * Wraps an API command function for use with a given output function.
 * @param args the command arguments
 * @param handler the handler function to call
 * @param output_func the function to call to output data, or NULL if no output is expected
 * @param output_ctx the context to pass to the output function
 * @return the status code of the operation
 */
static i32 api_wrap_handler(const char *args, api_func handler, api_output_func output_func, void *output_ctx) {
    char *out = NULL;
    i32 res = handler(args, &out);
    if (out && output_func) {
        output_func(output_ctx, "%s\n", out);
        free(out);
        return res == 200 ? -1 : res;
    }
    if (out) {
        free(out);
    }
    if (res != 200 || (output_func && !out)) {
        // Command failed, or output was expected but not produced
        return res;
    }
    return res;
}

i32 api_handle_get(const char *cmd, const char *args, api_output_func output_func, void *output_ctx) {
    if (strcasecmp(cmd, "GET_CALIBRATION") == 0) {
        return api_wrap_handler(args, api_get_calibration, output_func, output_ctx);
    } else if (strcasecmp(cmd, "GET_CONFIG") == 0) {
        return api_wrap_handler(args, api_get_config, output_func, output_ctx);
    } else if (strcasecmp(cmd, "GET_FLIGHTPLAN") == 0) {
        return api_wrap_handler(args, api_get_flightplan, output_func, output_ctx);
    } else if (strcasecmp(cmd, "GET_INFO") == 0) {
        return api_wrap_handler(args, api_get_info, output_func, output_ctx);
    } else if (strcasecmp(cmd, "GET_INPUT") == 0) {
        return api_wrap_handler(args, api_get_input, output_func, output_ctx);
    } else if (strcasecmp(cmd, "GET_LOGS") == 0) {
        return api_wrap_handler(args, api_get_logs, output_func, output_ctx);
    } else if (strcasecmp(cmd, "GET_MODE") == 0) {
        return api_wrap_handler(args, api_get_mode, output_func, output_ctx);
    } else if (strcasecmp(cmd, "GET_SENSOR") == 0) {
        return api_wrap_handler(args, api_get_sensor, output_func, output_ctx);
    } else {
        return 404;
    }
}

i32 api_handle_set(const char *cmd, const char *args, api_output_func output_func, void *output_ctx) {
    if (strcasecmp(cmd, "SET_ACTIVE") == 0) {
        return api_wrap_handler(args, api_set_active, output_func, output_ctx);
    } else if (strcasecmp(cmd, "SET_BAY") == 0) {
        return api_wrap_handler(args, api_set_bay, NULL, NULL);
    } else if (strcasecmp(cmd, "SET_CALIBRATION") == 0) {
        return api_wrap_handler(args, api_set_calibration, output_func, output_ctx);
    } else if (strcasecmp(cmd, "SET_CONFIG") == 0) {
        return api_wrap_handler(args, api_set_config, output_func, output_ctx);
    } else if (strcasecmp(cmd, "SET_CONFIG_FULL") == 0) {
        return api_wrap_handler(args, api_set_config_full, output_func, output_ctx);
    } else if (strcasecmp(cmd, "SET_FLIGHTPLAN") == 0) {
        return api_wrap_handler(args, api_set_flightplan, output_func, output_ctx);
    } else if (strcasecmp(cmd, "SET_MODE") == 0) {
        return api_wrap_handler(args, api_set_mode, NULL, NULL);
    } else if (strcasecmp(cmd, "SET_TARGET") == 0) {
        return api_wrap_handler(args, api_set_target, NULL, NULL);
    } else if (strcasecmp(cmd, "SET_WAYPOINT") == 0) {
        return api_wrap_handler(args, api_set_waypoint, NULL, NULL);
    } else {
        return 404;
    }
}

i32 api_handle_test(const char *cmd, const char *args) {
    if (strcasecmp(cmd, "TEST_ALL") == 0) {
        return api_test_all(args);
    } else if (strcasecmp(cmd, "TEST_GPS") == 0) {
        return api_test_gps(args);
    } else if (strcasecmp(cmd, "TEST_IMU") == 0) {
        return api_test_imu(args);
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
