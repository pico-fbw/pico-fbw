/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/flash.h"

// This file contains block device I/O functions for the littlefs file system, which is mainly used to store configuration
// files. Many popular platforms already have a littlefs driver, so look around online first! If not, writing your own driver
// isn't too hard, you just need to implement the following functions and configure the lfs_config struct. See
// https://github.com/littlefs-project/littlefs/issues/448 and https://github.com/littlefs-project/littlefs/blob/master/lfs.h
// for more information.

static int flash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
}

static int flash_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
}

static int flash_erase(const struct lfs_config *c, lfs_block_t block) {
}

static int flash_sync(const struct lfs_config *c) {
}

bool flash_setup() {
    // This function isn't part of littlefs.
    // It's run before any littlefs code during pico-fbw's boot process.
    // It has been given for convienience, so you can do any platform-specific setup here.
    // If your platform doesn't need to do anything, you can simply return true.
}

// clang-format off
struct lfs_config lfs_cfg = {
    .read = flash_read,
    .prog = flash_prog,
    .erase = flash_erase,
    .sync = flash_sync,
    .read_size = (x),
    .prog_size = (x),
    .block_size = (x),
    .block_count = (x),
    .cache_size = (x),
    .lookahead_size = (x),
    .block_cycles = (x)
};
// clang-format on

lfs_t lfs;
