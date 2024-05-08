#pragma once

#include "platform/defs.h" // Some platforms may define LFS_ config macros in their defs; this will make sure they're respected

#ifdef NDEBUG
    // Disable littlefs messages in release builds
    #define LFS_NO_DEBUG
    #define LFS_NO_WARN
    #define LFS_NO_ERROR
#endif

// littlefs header is required for lfs struct definitions, but fbw_lib is not included in CMake as we don't require any actual
// littlefs code
#include "lib/lfs.h"

/**
 * Setup the flash memory for use with littlefs.
 * @return true if successful
 * @note This function only runs any necessary platform-specific setup, the actual filesystem is not mounted.
 */
bool flash_setup();

/* TWO filesystems? Why?
Well, at least for most platforms, the files for pico-fbw's web interface are prebuilt and then compiled into the
final binary that you flash to your device. This is notable because, well, you're flashing the device.
That means overwriting whatever data may be there already. And it would kind of suck if all of your config
and calibration data was erased every time you updated to the latest version.

So for this reason, we have two filesystems! One (wwwfs) stores the aforementioned precompiled web assets,
and is overwritten every time an update is flashed (this means we can also flash web updates).
The other (lfs) is not overwritten when being flashed, so data persists between updates.
It's used to store, well, persistant data, such as config, calibration, logs, and more. */

#if PLATFORM_SUPPORTS_WIFI
extern lfs_t wwwfs;
extern struct lfs_config wwwfs_cfg;
#endif

extern lfs_t lfs;
extern struct lfs_config lfs_cfg;
