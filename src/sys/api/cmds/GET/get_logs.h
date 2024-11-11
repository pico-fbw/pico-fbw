#pragma once

#include "platform/types.h"

/**
 * Internal use version of the API command GET_LOGS, which returns output directly.
 * @param in unused
 * @param out pointer to where the output should be stored, allocated by the function
 * @return the status code of the operation
 * @note The caller is responsible for freeing the memory allocated for the output.
 * Both json_free_serialized_string() and free() can be used.
 */
i32 api_handle_get_logs(const char *in, char **out);

i32 api_get_logs(const char *args);
