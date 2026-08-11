#pragma once

#include "platform/types.h"

// Import all command headers so other files can import all commands simply by importing this file
#include "GET/get.h"
#include "MISC/misc.h"
#include "SET/set.h"
#include "TEST/test.h"

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
