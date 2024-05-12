#pragma once

#include <stdbool.h>
#include "platform/types.h"

#ifndef PICO_FBW_VERSION // Should be defined in the root CMakeLists.txt
    #define PICO_FBW_VERSION "Unknown"
#endif

#define PICO_FBW_API_VERSION "1.0"
#define FLIGHTPLAN_VERSION "1.0" // Version of flightplan that this version supports

typedef enum VersionCheck {
    VERSION_SAME,
    VERSION_NEWER,
    VERSION_OLDER,
    VERSION_NONE,
    VERSION_ERROR,
} VersionCheck;

/**
 * Checks the version of pico-fbw (this binary) against the current one (stored in flash).
 * @param vstr the version string to check against, or NULL to check against the current version in flash
 * @return VERSION_SAME if the versions match,
 * VERSION_NEWER if the version is newer (pre-release),
 * VERSION_OLDER if the version is older,
 * VERSION_NONE if no version was found to compare against (didn't exist in flash),
 * or VERSION_ERROR if there was an error (parse, filesystem).
 * @note If no `vstr` is specified (""), the version read from flash will used (and will be copied into `vstr`).
 * `vstr` should be of at least length 64, and should not be NULL.
 */
VersionCheck version_check(char vstr[]);

/**
 * Saves the current version of pico-fbw (this binary) to flash.
 * @return Whether the version was saved successfully.
 */
bool version_save();
