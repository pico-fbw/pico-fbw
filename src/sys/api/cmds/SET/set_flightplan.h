#pragma once

#include "platform/types.h"

/**
 * Internal use version of the API command SET_FLIGHTPLAN.
 * @param input the input to the command, same as it would be passed to the API
 * @return the status code of the operation
 */
i32 api_handle_set_flightplan(const char *input);

i32 api_set_flightplan(const char *args);
