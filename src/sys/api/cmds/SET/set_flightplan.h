#pragma once

#include "platform/types.h"

/**
 * Internal use version of the API command SET_FLIGHTPLAN.
 * @param input the input to the command, same as it would be passed to the API
 * @param output pointer to where the output should be stored, allocated by the function
 * @return the status code of the operation
 * @note The caller is responsible for freeing the memory allocated for the output.
 * Both json_free_serialized_string() and free() can be used.
 */
i32 api_handle_set_flightplan(const char *input, char **output);

i32 api_set_flightplan(const char *args);
