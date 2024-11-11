#pragma once

#include "platform/types.h"

/**
 * Internal use version of the API command SET_FLIGHTPLAN, which returns output directly.
 * @param in input to the command as it would be passed to the API
 * @param out unused
 * @return the status code of the operation
 */
i32 api_handle_set_config(const char *in, char **out);

i32 api_set_config(const char *args);
