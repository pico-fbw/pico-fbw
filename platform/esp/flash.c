/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU AGPL-3.0
 */

#include "platform/flash.h"

static int flash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {}

static int flash_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {}

static int flash_erase(const struct lfs_config *c, lfs_block_t block) {}

static int flash_sync(const struct lfs_config *c) {}

// struct lfs_config lfs_cfg = {
//     .read = flash_read,
//     .prog = flash_prog,
//     .erase = flash_erase,
//     .sync = flash_sync,
//     .read_size = 1,
//     .prog_size = FLASH_PAGE_SIZE,               // Minimum write size (256 bytes)
//     .block_size = FLASH_SECTOR_SIZE,            // Block size must be a multiple of the sector size (4096 bytes)
//     .block_count = FS_SIZE / FLASH_SECTOR_SIZE, // Number of blocks in the filesystem (64 blocks make up 256K FS_SIZE)
//     .cache_size = FLASH_SECTOR_SIZE / 4,        // Must be a multiple of the block size (1024 bytes)
//     .lookahead_size = 32,
//     .block_cycles = 500};

struct lfs_config lfs_cfg = {.read = flash_read,
                             .prog = flash_prog,
                             .erase = flash_erase,
                             .sync = flash_sync,
                             .read_size = 1,
                             .prog_size = 1u << 8,   // Minimum write size (256 bytes)
                             .block_size = 1u << 12, // Block size must be a multiple of the sector size (4096 bytes)
                             .block_count = (256 * 1024) /
                                            (1u << 12), // Number of blocks in the filesystem (64 blocks make up 256K FS_SIZE)
                             .cache_size = (1u << 12) / 4, // Must be a multiple of the block size (1024 bytes)
                             .lookahead_size = 32,
                             .block_cycles = 500};

lfs_t lfs;
