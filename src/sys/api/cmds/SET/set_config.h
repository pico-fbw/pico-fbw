#pragma once

#include "platform/types.h"

/**
 * Internal use version of the API command SET_CONFIG.
 * @param input the input to the command, same as it would be passed to the API
 * @return the status code of the operation
 */
i32 api_handle_set_config(const char *input);

i32 api_set_config(const char *args);
