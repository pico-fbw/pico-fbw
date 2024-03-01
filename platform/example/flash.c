/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/flash.h"

static int flash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {}

static int flash_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {}

static int flash_erase(const struct lfs_config *c, lfs_block_t block) {}

static int flash_sync(const struct lfs_config *c) {}

struct lfs_config lfs_cfg = {.read = flash_read,
                             .prog = flash_prog,
                             .erase = flash_erase,
                             .sync = flash_sync,
                             .read_size = (x),
                             .prog_size = (x),
                             .block_size = (x).block_count = (x),
                             .cache_size = (x),
                             .lookahead_size = (x),
                             .block_cycles = (x)};

lfs_t lfs;
