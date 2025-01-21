#pragma once

#include "platform/types.h"

/**
 * Executes the SET_TARGET API command.
 * @param in input to the command
 * @param out unused
 * @return the status code of the operation
 */
i32 api_set_target(const char *in, char **out);
