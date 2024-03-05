#pragma once

// littlefs header is required for the lfs_config struct, but fbw_lib is not included in CMake as we don't require any actual
// littlefs code
#include "lib/lfs.h"

/**
 * Setup the flash memory for use with littlefs.
 * @return true if successful
 * @note This function only runs any necessary platform-specific setup, the actual filesystem is not mounted.
 */
bool flash_setup();

extern lfs_t lfs;
extern struct lfs_config lfs_cfg;
