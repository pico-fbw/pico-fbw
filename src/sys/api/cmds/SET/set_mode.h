#pragma once

#include "platform/types.h"

/**
 * Executes the SET_MODE API command.
 * @param in input to the command
 * @param out unused
 * @return the status code of the operation
 */
i32 api_set_mode(const char *in, char **out);
