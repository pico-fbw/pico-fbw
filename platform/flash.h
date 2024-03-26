#pragma once

#include "platform/defs.h" // Some platforms may define LFS_ config macros in their defs; this will make sure they're respected

#ifdef NDEBUG
    #define LFS_NO_DEBUG // Disable littlefs debug messages in release builds
#endif

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

// TODO: have 2 littlefs's (and configs); one for pico-fbw's data and one for the www data
// (so that www updates can be pushed without overwriting config data)
