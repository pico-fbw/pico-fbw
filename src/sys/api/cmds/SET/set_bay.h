#pragma once

#include "platform/types.h"

/**
 * Executes the SET_BAY API command.
 * @param in input to the command
 * @param out unused
 * @return the status code of the operation
 */
i32 api_set_bay(const char *in, char **out);
