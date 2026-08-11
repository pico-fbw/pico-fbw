#pragma once

#include "platform/types.h"

/**
 * Executes the respective API command.
 * @param in input to the command
 * @param out pointer to where the output should be stored, allocated by the function
 * @return the status code of the operation
 * @note The caller is responsible for freeing the memory allocated for the output.
 * Both json_free_serialized_string() and free() may be used.
 */

i32 api_get_calibration(const char *in, char **out);
i32 api_get_config(const char *in, char **out);
i32 api_get_flightplan(const char *in, char **out);
i32 api_get_info(const char *in, char **out);
i32 api_get_input(const char *in, char **out);
i32 api_get_logs(const char *in, char **out);
i32 api_get_mode(const char *in, char **out);
i32 api_get_sensor(const char *in, char **out);
