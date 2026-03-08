#pragma once

#include "platform/types.h"

/**
 * Executes the SET_CALIBRATION API command.
 * @param in input to the command
 * @param out pointer to where the output should be stored, allocated by the function.
 * @return the status code of the operation
 * @note The caller is responsible for freeing the memory allocated for the output.
 * Both json_free_serialized_string() and free() may be used.
 */
i32 api_set_calibration(const char *in, char **out);
