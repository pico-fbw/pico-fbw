#pragma once

#include "platform/types.h"

// Import all command headers so other files can import all commands simply by importing this file

#include "GET/get_calibration.h"
#include "GET/get_config.h"
#include "GET/get_flightplan.h"
#include "GET/get_info.h"
#include "GET/get_input.h"
#include "GET/get_logs.h"
#include "GET/get_mode.h"
#include "GET/get_sensor.h"

#include "SET/set_active.h"
#include "SET/set_bay.h"
#include "SET/set_calibration.h"
#include "SET/set_config.h"
#include "SET/set_flightplan.h"
#include "SET/set_mode.h"
#include "SET/set_target.h"
#include "SET/set_waypoint.h"

#include "TEST/test_all.h"
#include "TEST/test_gps.h"
#include "TEST/test_imu.h"
#include "TEST/test_pwm.h"
#include "TEST/test_servo.h"
#include "TEST/test_throttle.h"

#include "MISC/about.h"
#include "MISC/help.h"
#include "MISC/ping.h"
#include "MISC/reboot.h"
#include "MISC/reset.h"

// Type for an API output function
typedef int (*api_output_func)(void *ctx, const char *fmt, ...);

/**
 * Handles API GET commands.
 * @param cmd the command
 * @param args the command arguments
 * @param output_func function to call on production of output data
 * @param output_ctx context to pass to output_func
 * @return the status code
 */
i32 api_handle_get(const char *cmd, const char *args, api_output_func output_func, void *output_ctx);

/**
 * Handles API SET commands.
 * @param cmd the command
 * @param args the command arguments
 * @param output_func function to call on production of output data
 * @param output_ctx context to pass to output_func
 * @return the status code
 */
i32 api_handle_set(const char *cmd, const char *args, api_output_func output_func, void *output_ctx);

/**
 * Handles API TEST commands.
 * @param cmd the command
 * @param args the command arguments
 * @return the status code
 */
i32 api_handle_test(const char *cmd, const char *args);

/**
 * Handles API MISC (no prefix) commands.
 * @param cmd the command
 * @param args the command arguments
 * @return either status code 404, or -1 if the command was successful (MISC commands don't have return codes)
 */
i32 api_handle_misc(const char *cmd, const char *args);
