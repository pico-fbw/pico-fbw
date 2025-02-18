#pragma once

#include "platform/types.h"

// cmds.h is included for convenience (so other files can import all commands with a single include)
#include "sys/api/cmds/cmds.h"

// Type for API command functions
typedef i32 (*api_func)(const char *in, char **out);

/**
 * Polls the API for new data (incoming commands) and responds if necessary.
 * @return the status code of the executed command, or 0 if no command was executed
 */
i32 api_poll();

/**
 * Converts an API response code to an HTTP status code.
 * @param res the API response code
 * @return the HTTP status code (as a string)
 */
const char *api_res_to_http_status(i32 res);
