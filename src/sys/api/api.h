#pragma once

#include "platform/types.h"

// Type for internal API command handler functions.
typedef i32 (*api_handler)(const char *in, char **out);

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
