#pragma once

// littlefs header is required for the lfs_config struct, but fbw_lib is not included in CMake as we don't require any actual littlefs code
#include "lib/lfs.h"

extern lfs_t lfs;
extern struct lfs_config lfs_cfg;
