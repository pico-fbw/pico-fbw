#pragma once

#include <stdbool.h>
#include "platform/int.h"

#define PICO_FBW_VERSION "1.0.0-alpha.3"
#ifdef NDEBUG
#define DEBUG_BUILD false
#else
#define DEBUG_BUILD true
#endif

#define PICO_FBW_API_VERSION "1.0"
#define FPLAN_VERSION "1.0"

/**
 * Checks the version of pico-fbw (this binary) against the current one (stored in flash).
 * @param vstr the version string to check against, or NULL to check against the current version in flash
 * @return 0 if the versions match,
 * 1 if the version is newer (pre-release),
 * -1 if the version is older,
 * -2 if there was no valid version to parse,
 * -3 if there was a parse error,
 * or -4 if there was a filesystem error.
 * @note If no `vstr` is specified, the version read from flash will be copied into `vstr`.
 */
i32 version_check(char vstr[]);

/**
 * Saves the current version of pico-fbw (this binary) to flash.
 * @return Whether the version was saved successfully.
 */
bool version_save();
